#include "common/global/global.hpp"
#include "out_of_order/ooo_match.hpp"
#include "sched/entry_factory.hpp"

struct ooo_runtime_info
{
    size_t match_id_size_buffer;
    //root ranks reserved id and bcasts it to other ranks
    mlsl_comm_id_t reserved_id;
    //preallocated and filled with actual data on the root rank
    //allocated after the 1st bcast on other ranks
    void* match_id_name;
    mlsl_sched* sched;
    out_of_order::ooo_match* ooo_handler;
};

out_of_order::ooo_match::ooo_match(mlsl_executor& exec,
                                   comm_id_storage& comm_ids)
    : executor(exec), comm_ids(comm_ids)
{
    MLSL_LOG(INFO, "Configuring out-of-order collectives support");

    auto id = std::unique_ptr<comm_id>(new comm_id(comm_ids, true));
    service_comm = std::make_shared<mlsl_comm>(executor.proc_idx, executor.proc_count, std::move(id));
}

mlsl_comm* out_of_order::ooo_match::get_user_comm(const std::string& match_id)
{
    std::lock_guard<mlsl_spinlock> lock{postponed_user_scheds_guard};

    //return communicator for the first (and may be only) schedule.
    auto first_schedule = postponed_user_scheds.find(match_id);
    if (first_schedule != postponed_user_scheds.end())
    {
        return first_schedule->second->coll_param.comm;
    }
    return nullptr;
}

void out_of_order::ooo_match::postpone(mlsl_sched* sched)
{
    const std::string& match_id = sched->coll_attr.match_id;

    //1. create and start bcast of sched match_id
    bcast_match_id(sched->coll_attr.match_id);

    //2. Check if there is unresolved match_id
    auto unresolved = unresolved_comms.find(match_id);
    if (unresolved != unresolved_comms.end())
    {
        MLSL_LOG(DEBUG, "Found postponed comm creation for match_id %s, id %hu", match_id.c_str(),
                 unresolved->second->value());
        //2.2. create new communicator with reserved comm_id
        auto match_id_comm = sched->coll_param.comm->clone_with_new_id(std::move(unresolved->second));

        add_comm(match_id, match_id_comm);
        update_comm_and_run(sched, match_id_comm.get());
        unresolved_comms.erase(unresolved);
    }
    else
    {
        //3. postpone this schedule
        MLSL_LOG(INFO, "Postpone sched %s %p for match_id %s", mlsl_coll_type_to_str(sched->coll_param.ctype), sched,
                 match_id.c_str());
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

    MLSL_LOG(INFO, "creating comm id %hu for match_id %s", id->value(), match_id.c_str());
    //get user provided communicator from the postponed schedule
    auto original_comm = get_user_comm(match_id);
    if (!original_comm)
    {
        //root ranks has broadcasted match_id, but other ranks have not received request
        //from the user yet, so we can't create a communicator. Need to wait for request from the user.
        MLSL_LOG(DEBUG, "Can't find postponed sched for match_id %s, postpone comm creation, reserved id %hu",
                 match_id.c_str(), id->value());
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
        MLSL_LOG(DEBUG, "bcast of %s is already in progress", match_id.c_str());
        return;
    }

    mlsl_coll_param bcast_param{};
    bcast_param.ctype = mlsl_coll_none;
    bcast_param.dtype = mlsl_dtype_internal_char;
    bcast_param.comm = service_comm.get();
    auto bcast_sched = new mlsl_sched(bcast_param);
    bcast_sched->is_internal = true;

    MLSL_LOG(DEBUG, "Building service sched %p, req %p", bcast_sched, bcast_sched->req);

    //1. broadcast match_id length
    auto ctx = static_cast<ooo_runtime_info*>(bcast_sched->alloc_buffer(sizeof(ooo_runtime_info)));
    ctx->sched = bcast_sched;
    ctx->ooo_handler = this;

    if (service_comm->rank() == 0)
    {
        MLSL_THROW_IF_NOT(!match_id.empty(), "root rank can't bcast empty match_id");
        ctx->match_id_size_buffer = match_id.length();
        ctx->match_id_name = bcast_sched->alloc_buffer(ctx->match_id_size_buffer + 1);
        strcpy(static_cast<char*>(ctx->match_id_name), match_id.c_str());
        ctx->reserved_id = comm_ids.acquire_id(true);
        MLSL_LOG(INFO, "root bcasts match_id %s, comm id %hu", match_id.c_str(), ctx->reserved_id);
    }

    entry_factory::make_coll_entry(bcast_sched,
                                   mlsl_coll_bcast,
                                   nullptr,
                                   &ctx->match_id_size_buffer,
                                   sizeof(size_t),
                                   mlsl_dtype_internal_char,
                                   mlsl_reduction_custom);

    bcast_sched->add_barrier();

    //2. broadcast match_id
    auto bcast_value_entry = entry_factory::make_coll_entry(bcast_sched,
                                                            mlsl_coll_bcast,
                                                            nullptr,
                                                            nullptr, //postponed
                                                            0,       //postponed
                                                            mlsl_dtype_internal_char,
                                                            mlsl_reduction_custom);

    //update count field of match_id bcast entry
    bcast_value_entry->set_field_fn(mlsl_sched_entry_field_cnt, [](const void* ctx,
                                                                   void* field) -> mlsl_status_t
    {
        auto rt_info = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        auto bcast_size = static_cast<size_t*>(field);
        *bcast_size = rt_info->match_id_size_buffer;
        return mlsl_status_success;
    },
    ctx);

    //update buf field of match_id bcast entry
    bcast_value_entry->set_field_fn(mlsl_sched_entry_field_buf, [](const void* ctx,
                                                                   void* field)
    {
        auto rt_info = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        if (rt_info->sched->coll_param.comm->rank() != 0)
        {
            //root rank allocates and fills this buffer during schedule creation
            rt_info->match_id_name = rt_info->sched->alloc_buffer(rt_info->match_id_size_buffer);
        }
        //actually this is double ptr
        void** bcast_buffer = reinterpret_cast<void**>(field);
        *bcast_buffer = rt_info->match_id_name;
        return mlsl_status_success;
    },
    ctx);

    bcast_sched->add_barrier();

    //3. broadcast reserved id
    entry_factory::make_coll_entry(bcast_sched,
                                   mlsl_coll_bcast,
                                   nullptr,
                                   &ctx->reserved_id,
                                   sizeof(mlsl_comm_id_t),
                                   mlsl_dtype_internal_char,
                                   mlsl_reduction_custom);

    bcast_sched->add_barrier();

    //4. create a communicator
    entry_factory::make_function_entry(bcast_sched, [](const void* ctx) -> mlsl_status_t
    {
        auto ooo_ctx = static_cast<ooo_runtime_info*>(const_cast<void*>(ctx));
        ooo_ctx->ooo_handler->create_comm_and_run_sched(ooo_ctx);
        return mlsl_status_success;
    },
    ctx);

    bcast_sched->commit(nullptr);

    bcast_sched->start(&executor);
}

void out_of_order::ooo_match::update_comm_and_run(mlsl_sched* sched,
                                                  mlsl_comm* comm)
{
    sched->coll_param.comm = comm;
    for (size_t part_idx = 0; part_idx < sched->partial_scheds.size(); ++part_idx)
    {
        sched->partial_scheds[part_idx]->coll_param.comm = comm;
    }
    sched->start(&executor, false);
}

mlsl_comm* out_of_order::ooo_match::get_comm(const std::string& match_id)
{
    std::lock_guard<mlsl_spinlock> lock(match_id_to_comm_map_guard);
    auto comm = match_id_to_comm_map.find(match_id);
    if (comm != match_id_to_comm_map.end())
    {
        MLSL_LOG(DEBUG, "comm for match_id %s has been found", match_id.c_str());
        return comm->second.get();
    }
    MLSL_LOG(DEBUG, "no comm for match_id %s has been found", match_id.c_str());
    return nullptr;
}

void out_of_order::ooo_match::add_comm(std::string match_id,
                                       std::shared_ptr<mlsl_comm> comm)
{
    std::lock_guard<mlsl_spinlock> lock(match_id_to_comm_map_guard);
    if (match_id_to_comm_map.find(match_id) != match_id_to_comm_map.end())
    {
        MLSL_LOG(ERROR, "match_id %s already exists", match_id.c_str());
        return;
    }
    match_id_to_comm_map.emplace(match_id, comm);
}

bool out_of_order::ooo_match::is_bcast_in_progress(const std::string& match_id)
{
    std::lock_guard<mlsl_spinlock> lock{postponed_user_scheds_guard};
    return postponed_user_scheds.count(match_id) != 0;
}

void out_of_order::ooo_match::postpone_user_sched(mlsl_sched* sched)
{
    std::lock_guard<mlsl_spinlock> lock{postponed_user_scheds_guard};
    MLSL_LOG(DEBUG, "Storage contains %zu entries for match_id %s",
             postponed_user_scheds.count(sched->coll_attr.match_id),
             sched->coll_attr.match_id.c_str());
    postponed_user_scheds.emplace(sched->coll_attr.match_id, sched);
}

void out_of_order::ooo_match::run_postponed(const std::string& match_id,
                                            mlsl_comm* match_id_comm)
{
    MLSL_THROW_IF_NOT(match_id_comm, "incorrect comm");

    std::lock_guard<mlsl_spinlock> lock{postponed_user_scheds_guard};
    auto scheds = postponed_user_scheds.equal_range(match_id);

    MLSL_LOG(DEBUG, "Found %td scheds for match_id %s", std::distance(scheds.first, scheds.second),
             match_id.c_str());

    if (scheds.first != postponed_user_scheds.end() || scheds.second != postponed_user_scheds.end())
    {
        for (auto sched_it = scheds.first; sched_it != scheds.second; ++sched_it)
        {
            MLSL_LOG(INFO, "Running sched %p, type %s for match_id %s", sched_it->second,
                     mlsl_coll_type_to_str(sched_it->second->coll_param.ctype), sched_it->first.c_str());
            //user who started postponed collective has already had a request object
            mlsl_sched* sched_obj = sched_it->second;
            update_comm_and_run(sched_obj, match_id_comm);
        }

        MLSL_LOG(DEBUG, "Erase postponed scheds for match_id %s if any", match_id.c_str());
        postponed_user_scheds.erase(scheds.first, scheds.second);
    }
}

