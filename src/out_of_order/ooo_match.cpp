#include "common/global/global.hpp"
#include "out_of_order/ooo_match.hpp"
#include "sched/entry_factory.hpp"

#include <cstring>

struct ooo_runtime_info
{
    size_t match_id_size_buffer;
    //root ranks reserved id and bcasts it to other ranks
    ccl_comm_id_t reserved_id;
    //preallocated and filled with actual data on the root rank
    //allocated after the 1st bcast on other ranks
    void* match_id_name;
    ccl_sched* sched;
    out_of_order::ooo_match* ooo_handler;
};

out_of_order::ooo_match::ooo_match(ccl_executor& exec,
                                   ccl_comm_id_storage& comm_ids)
    : executor(exec), comm_ids(comm_ids)
{
    LOG_INFO("Configuring out-of-order collectives support");

    auto id = std::unique_ptr<comm_id>(new comm_id(comm_ids, true));
    service_comm = std::make_shared<ccl_comm>(executor.proc_idx, executor.proc_count, std::move(id));
}

ccl_comm* out_of_order::ooo_match::get_user_comm(const std::string& match_id)
{
    std::lock_guard<ccl_spinlock> lock{postponed_user_scheds_guard};

    //return communicator for the first (and may be only) schedule.
    auto first_schedule = postponed_user_scheds.find(match_id);
    if (first_schedule != postponed_user_scheds.end())
    {
        return first_schedule->second->coll_param.comm;
    }
    return nullptr;
}

void out_of_order::ooo_match::postpone(ccl_sched* sched)
{
    const std::string& match_id = sched->coll_attr.match_id;

    //1. create and start bcast of sched match_id
    bcast_match_id(sched->coll_attr.match_id);

    //2. Check if there is unresolved match_id
    auto unresolved = unresolved_comms.find(match_id);
    if (unresolved != unresolved_comms.end())
    {
        LOG_DEBUG("found postponed comm creation for match_id ", match_id, ", id ", unresolved->second->value());
        //2.2. create new communicator with reserved comm_id
        auto match_id_comm = sched->coll_param.comm->clone_with_new_id(std::move(unresolved->second));

        add_comm(match_id, match_id_comm);
        update_comm_and_run(sched, match_id_comm.get());
        unresolved_comms.erase(unresolved);
    }
    else
    {
        //3. postpone this schedule
        LOG_INFO("postpone sched ", ccl_coll_type_to_str(sched->coll_param.ctype), " ", sched, " for match_id ",
                 match_id);
        postpone_user_sched(sched);
    }
}

void out_of_order::ooo_match::create_comm_and_run_sched(ooo_runtime_info* ctx)
{
    if (service_comm->rank() != 0)
    {
        //non-root ranks have not allocated comm_id yet
        comm_ids.pull_id(ctx->reserved_id);
    }
    auto id = std::unique_ptr<comm_id>(new comm_id(comm_ids, ctx->reserved_id));

    std::string match_id{static_cast<const char*>(ctx->match_id_name)};

    LOG_INFO("creating comm id ", id->value(), " for match_id ", match_id);
    //get user provided communicator from the postponed schedule
    auto original_comm = get_user_comm(match_id);
    if (!original_comm)
    {
        //root ranks has broadcasted match_id, but other ranks have not received request
        //from the user yet, so we can't create a communicator. Need to wait for request from the user.
        LOG_DEBUG("Can't find postponed sched for match_id ", match_id, ", postpone comm creation, reserved id ",
            id->value());
        unresolved_comms.emplace(std::move(match_id), std::move(id));
        return;
    }

    auto match_id_comm = original_comm->clone_with_new_id(std::move(id));

    run_postponed(match_id, match_id_comm.get());
    add_comm(std::move(match_id), match_id_comm);
}

void out_of_order::ooo_match::bcast_match_id(const std::string& match_id)
{
    if (is_bcast_in_progress(match_id))
    {
        LOG_DEBUG("bcast of ", match_id, " is already in progress");
        return;
    }

    ccl_coll_param bcast_param{};
    bcast_param.ctype = ccl_coll_none;
    bcast_param.dtype = ccl_dtype_internal_char;
    bcast_param.comm = service_comm.get();
    auto bcast_sched = new ccl_sched(bcast_param);
    bcast_sched->internal_type = ccl_sched_internal_ooo;

    LOG_DEBUG("Building service sched ", bcast_sched, ", req ", bcast_sched->req);

    //1. broadcast match_id length
    auto ctx = static_cast<ooo_runtime_info*>(bcast_sched->alloc_buffer(sizeof(ooo_runtime_info)).get_ptr());
    ctx->sched = bcast_sched;
    ctx->ooo_handler = this;

    if (service_comm->rank() == 0)
    {
        CCL_THROW_IF_NOT(!match_id.empty(), "root rank can't bcast empty match_id");
        ctx->match_id_size_buffer = match_id.length();
        ctx->match_id_name = bcast_sched->alloc_buffer(ctx->match_id_size_buffer + 1).get_ptr();
        strncpy(static_cast<char*>(ctx->match_id_name), match_id.c_str(), ctx->match_id_size_buffer);
        ctx->reserved_id = comm_ids.acquire_id(true);
        LOG_INFO("root bcasts match_id ", match_id, ", comm id ", ctx->reserved_id);
    }

    entry_factory::make_coll_entry(bcast_sched,
                                   ccl_coll_bcast,
                                   ccl_buffer(), /* unused */
                                   ccl_buffer(&ctx->match_id_size_buffer, sizeof(size_t)),
                                   sizeof(size_t),
                                   ccl_dtype_internal_char,
                                   ccl_reduction_custom);

    bcast_sched->add_barrier();

    //2. broadcast match_id
    auto bcast_value_entry = entry_factory::make_coll_entry(bcast_sched,
                                                            ccl_coll_bcast,
                                                            ccl_buffer(), /* unused */
                                                            ccl_buffer(), /* postponed */
                                                            0,             /* postponed */
                                                            ccl_dtype_internal_char,
                                                            ccl_reduction_custom);

    //update count field of match_id bcast entry
    bcast_value_entry->set_field_fn(ccl_sched_entry_field_cnt, [](const void* ctx,
                                                                   void* field) -> ccl_status_t
    {
        auto rt_info = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        auto bcast_size = static_cast<size_t*>(field);
        *bcast_size = rt_info->match_id_size_buffer;
        return ccl_status_success;
    },
    ctx);

    //update buf field of match_id bcast entry
    bcast_value_entry->set_field_fn(ccl_sched_entry_field_buf, [](const void* ctx,
                                                                   void* field)
    {
        auto rt_info = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        if (rt_info->sched->coll_param.comm->rank() != 0)
        {
            //root rank allocates and fills this buffer during schedule creation
            rt_info->match_id_name = rt_info->sched->alloc_buffer(rt_info->match_id_size_buffer).get_ptr();
        }

        ccl_buffer* bcast_buffer = (ccl_buffer*)field;
        bcast_buffer->set(rt_info->match_id_name, rt_info->match_id_size_buffer);

        return ccl_status_success;
    },
    ctx);

    bcast_sched->add_barrier();

    //3. broadcast reserved id
    entry_factory::make_coll_entry(bcast_sched,
                                   ccl_coll_bcast,
                                   ccl_buffer(), /* unused */
                                   ccl_buffer(&ctx->reserved_id,
                                               sizeof(ccl_comm_id_t)),
                                   sizeof(ccl_comm_id_t),
                                   ccl_dtype_internal_char,
                                   ccl_reduction_custom);

    bcast_sched->add_barrier();

    //4. create a communicator
    entry_factory::make_function_entry(bcast_sched, [](const void* ctx) -> ccl_status_t
    {
        auto ooo_ctx = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        ooo_ctx->ooo_handler->create_comm_and_run_sched(ooo_ctx);
        return ccl_status_success;
    },
    ctx);

    bcast_sched->commit(nullptr);

    bcast_sched->start(&executor);
}

void out_of_order::ooo_match::update_comm_and_run(ccl_sched* sched,
                                                  ccl_comm* comm)
{
    sched->coll_param.comm = comm;
    for (size_t part_idx = 0; part_idx < sched->partial_scheds.size(); ++part_idx)
    {
        sched->partial_scheds[part_idx]->coll_param.comm = comm;
    }
    sched->start(&executor, false);
}

ccl_comm* out_of_order::ooo_match::get_comm(const std::string& match_id)
{
    std::lock_guard<ccl_spinlock> lock(match_id_to_comm_map_guard);
    auto comm = match_id_to_comm_map.find(match_id);
    if (comm != match_id_to_comm_map.end())
    {
        LOG_DEBUG("comm for match_id ", match_id, " has been found");
        return comm->second.get();
    }
    LOG_DEBUG("no comm for match_id ", match_id, " has been found");
    return nullptr;
}

void out_of_order::ooo_match::add_comm(std::string match_id,
                                       std::shared_ptr<ccl_comm> comm)
{
    std::lock_guard<ccl_spinlock> lock(match_id_to_comm_map_guard);
    if (match_id_to_comm_map.find(match_id) != match_id_to_comm_map.end())
    {
        LOG_ERROR("match_id ", match_id, " already exists");
        return;
    }
    match_id_to_comm_map.emplace(match_id, comm);
}

bool out_of_order::ooo_match::is_bcast_in_progress(const std::string& match_id)
{
    std::lock_guard<ccl_spinlock> lock{postponed_user_scheds_guard};
    return postponed_user_scheds.count(match_id) != 0;
}

void out_of_order::ooo_match::postpone_user_sched(ccl_sched* sched)
{
    std::lock_guard<ccl_spinlock> lock{postponed_user_scheds_guard};
    LOG_DEBUG("Storage contains ", postponed_user_scheds.count(sched->coll_attr.match_id), " entries for match_id ",
             sched->coll_attr.match_id);
    postponed_user_scheds.emplace(sched->coll_attr.match_id, sched);
}

void out_of_order::ooo_match::run_postponed(const std::string& match_id,
                                            ccl_comm* match_id_comm)
{
    CCL_THROW_IF_NOT(match_id_comm, "incorrect comm");

    std::lock_guard<ccl_spinlock> lock{postponed_user_scheds_guard};
    auto scheds = postponed_user_scheds.equal_range(match_id);

    LOG_DEBUG("Found ", std::distance(scheds.first, scheds.second)," scheds for match_id ",
              match_id);

    if (scheds.first != postponed_user_scheds.end() || scheds.second != postponed_user_scheds.end())
    {
        for (auto sched_it = scheds.first; sched_it != scheds.second; ++sched_it)
        {
            LOG_INFO("Running sched ", sched_it->second,
                     ", type ", ccl_coll_type_to_str(sched_it->second->coll_param.ctype),
                     ",  for match_id %s", sched_it->first);
            //user who started postponed collective has already had a request object
            ccl_sched* sched_obj = sched_it->second;
            update_comm_and_run(sched_obj, match_id_comm);
        }

        LOG_DEBUG("Erase postponed scheds for match_id ", match_id);
        postponed_user_scheds.erase(scheds.first, scheds.second);
    }
}

