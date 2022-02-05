#include "coll/coll_check.hpp"
#include "common/global/global.hpp"
#include "common/utils/sync_object.hpp"
#include "common/utils/sycl_utils.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/cache/cache.hpp"
#include "sched/cache/key.hpp"
#include "sched/entry/factory/entry_factory.hpp"
#include "sched/sched.hpp"
#include "sched/queue/queue.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#include <CL/sycl/backend/level_zero.hpp>

#ifdef CCL_ENABLE_ZE
#include "sched/entry/ze/ze_cache.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

#endif // CCL_ENABLE_ZE
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_SYCL
constexpr ze_event_pool_desc_t get_event_pool_desc() {
    auto desc = ccl::ze::default_event_pool_desc;

    desc.count = 1;
    desc.flags = ZE_EVENT_POOL_FLAG_HOST_VISIBLE;

    return desc;
}
#endif

ccl_sched::ccl_sched(const ccl_sched_create_param& param,
                     ccl_request* master_request,
                     ccl_sched* master_sched)
        : ccl_sched_base(param),
          req(master_request),
          parent_sched(master_sched) {
    type = sched_type_t::regular;
    strict_order = ccl::global_data::env().enable_strict_order;
}

ccl_sched::ccl_sched(const ccl_sched_create_param& param, ccl_sched* master_sched)
        : ccl_sched_base(param),
          ccl_request(),
          req(this),
          parent_sched(master_sched) {
    type = sched_type_t::extra;
    strict_order = ccl::global_data::env().enable_strict_order;
#ifdef ENABLE_DEBUG
    set_dump_callback([this](std::ostream& out) {
        dump(out);
    });
#endif
}

ccl_sched::ccl_sched(const ccl_sched_create_param& param)
        : ccl_sched_base(param),
          ccl_request(),
          subscheds() {
    type = sched_type_t::master;
#ifdef ENABLE_DEBUG
    set_dump_callback([this](std::ostream& out) {
        dump(out);
    });
#endif

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (ccl::utils::should_use_sycl_output_event(coll_param.stream)) {
        auto ze_context = coll_param.stream->get_ze_context();
        auto pool_desc = get_event_pool_desc();
        ccl::global_data::get().ze_data->cache->get(
            0, ze_context, pool_desc, &get_memory().sync_pool);

        ze_event_desc_t event_desc = ccl::ze::default_event_desc;
        ZE_CALL(zeEventCreate, (get_memory().sync_pool, &event_desc, &get_memory().sync_event));
        LOG_DEBUG("created sync event: ", get_memory().sync_event);
    }
    else {
        LOG_DEBUG("skip sync event creation");
    }
#endif
}

ccl_sched::~ccl_sched() {
    if (in_bin_status == ccl_sched_in_bin_added)
        LOG_DEBUG("in_bin_status == ccl_sched_in_bin_added");

    if (finalize_fn) {
        finalize_fn(this, finalize_fn_ctx);
    }
    if (type != sched_type_t::master)
        return;

    for (auto& part_sched : subscheds) {
        part_sched.reset();
    }
    if (!memory.mr_list.empty())
        LOG_WARN("memory region list should be empty for master sched");

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (ccl::utils::should_use_sycl_output_event(coll_param.stream)) {
        // Sycl event might call wait on destruction meaning that it should be valid at that time
        // The problem is that the sync event is stored in request, which descrutor is called
        // after ccl_master_sched, which means its underlying l0 event will be already destroyed
        // by that time. As a workaround, reset the event, essentially calling its destructor before
        // destroying the corresponding l0 event
        set_sync_event(sycl::event());

        LOG_DEBUG("destroying sync event: ", get_memory().sync_event);
        ZE_CALL(zeEventDestroy, (get_memory().sync_event));

        auto ze_context = coll_param.stream->get_ze_context();
        auto pool_desc = get_event_pool_desc();
        ccl::global_data::get().ze_data->cache->push(
            0, ze_context, pool_desc, get_memory().sync_pool);
    }
    else {
        LOG_DEBUG("skip sync event destruction");
    }
#endif
}

void ccl_sched::commit(ccl_parallelizer* parallelizer, bool update_sched_id) {
    if (ccl::global_data::env().priority_mode == ccl_priority_lifo) {
        coll_attr.priority = ccl_sched_base::get_lifo_priority();
    }

    if (subscheds.empty()) {
        /* single time operations */
        if (update_sched_id) {
            update_id();
        }
        if (parallelizer) {
            parallelizer->process(this, update_sched_id);
            CCL_THROW_IF_NOT(
                !subscheds.empty(),
                "ccl_master_sched must have at least 1 partial sched after parallelized");
        }
    }
    else {
        /* repeated operations, should happen each time to reuse schedule */
        for (size_t idx = 0; idx < subscheds.size(); idx++) {
            subscheds[idx]->coll_attr.priority = coll_attr.priority;
        }
    }

    LOG_DEBUG("sched ",
              this,
              ", sched_id ",
              sched_id,
              ", req ",
              static_cast<const ccl_request*>(this),
              ", subscheds_count ",
              subscheds.size());
}

void ccl_sched::reset_state() {
    reset_request();

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (ccl::utils::should_use_sycl_output_event(coll_param.stream)) {
        // Reset sycl event while it's in complete state, similar case to destruction in ~ccl_master_sched
        set_sync_event(sycl::event());
        LOG_DEBUG("reset sync event: ", get_memory().sync_event);
        ZE_CALL(zeEventHostReset, (get_memory().sync_event));
    }
#endif
}

ccl_request* ccl_sched::start(ccl_executor* exec, bool reset_sched, bool update_sched_id) {
    /* sanity check the schedule */
    CCL_THROW_IF_NOT(coll_param.comm);

    LOG_DEBUG("starting schedule ", this, ", type ", ccl_coll_type_to_str(coll_param.ctype));

    prepare_subscheds(update_sched_id);

    if (reset_sched) {
        reset_state();
    }

    if (ccl::global_data::env().sched_dump) {
        std::stringstream ostream;
        dump(ostream);
        logger.info(ostream.str());
    }

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    if (ccl::utils::should_use_sycl_output_event(coll_param.stream)) {
        LOG_DEBUG("convert L0 event: ",
                  get_memory().sync_event,
                  "into a SYCL event and submit a barrier");
        auto q = coll_param.stream->get_native_stream();
        auto context = q.get_context();
#ifdef CCL_ENABLE_SYCL_INTEROP_EVENT
        auto e = ccl::utils::make_event(context, get_memory().sync_event);
        set_sync_event(e);
        set_native_event(ccl::utils::submit_barrier(q, e));
#else // CCL_ENABLE_SYCL_INTEROP_EVENT
        CCL_THROW("interop event functionality is not available with current configuration, "
                  "please rebuild oneCCL using ENABLE_SYCL_INTEROP_EVENT option "
                  "and a DPCPP compiler that supports that feature");
#endif // CCL_ENABLE_SYCL_INTEROP_EVENT
    }
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    exec->start(this);

    return this;
}

ccl_request* ccl_sched::reset_request() {
    set_counter(std::max(1, static_cast<int>(subscheds.size())));
    return this;
}

void ccl_sched::add_subsched(const ccl_coll_param& coll_param, bool update_sched_id) {
    subscheds.emplace_back(std::make_shared<ccl_sched>(
        ccl_sched_create_param(sched_type,
                               update_sched_id
                                   ? coll_param.comm->get_sched_id(sched_type != ccl_sched_regular)
                                   : sched_id,
                               coll_param),
        this,
        this));
}

std::vector<std::shared_ptr<ccl_sched>>& ccl_sched::get_subscheds() {
    return subscheds;
}

void ccl_sched::prepare_subscheds(bool update_sched_id) {
    for (auto& sched : subscheds) {
        sched->renew(update_sched_id);
    }
}

void ccl_sched::sync_subscheds() {
    CCL_THROW_IF_NOT(!subscheds.empty(), "no partial schedules");

    bool add_sync_entry = false;

    /* ensure all partial schedules have the same add_mode */
    ccl_sched_add_mode add_mode = subscheds[0]->get_add_mode();
    for (auto& sched : subscheds) {
        CCL_THROW_IF_NOT(sched->get_add_mode() == add_mode,
                         "unexpected add_mode ",
                         sched->get_add_mode(),
                         ", expected ",
                         add_mode);
    }

    /* check whether all partial schedules already have sync_entry at the tail */
    for (auto& sched : subscheds) {
        if (sched->entries.empty()) {
            add_sync_entry = true;
            break;
        }

        /* TODO: add enum field into base entry to distinguish different entry types */
        const char* tail_entry_name = (add_mode == ccl_sched_add_back)
                                          ? sched->entries.back()->name()
                                          : sched->entries.front()->name();

        if (tail_entry_name && strcmp(tail_entry_name, "SYNC")) {
            add_sync_entry = true;
            break;
        }
    }

    /* if at least one partial schedule doesn't have sync entry
       then sync all partial schedules */
    if (add_sync_entry) {
        auto sync_obj = std::make_shared<sync_object>(subscheds.size());
        for (auto& sched : subscheds) {
            entry_factory::create<sync_entry>(sched.get(), sync_obj);
        }
    }
}

void ccl_sched::dump(std::ostream& out) const {
    if (!ccl::global_data::env().sched_dump) {
        return;
    }

    ccl_sched_base::dump(out, class_name());
    ccl_logger::format(out,
                       ", start_idx: ",
                       start_idx,
                       ", req: ",
                       static_cast<const ccl_request*>(this),
                       ", num_entries: ",
                       entries.size(),
                       ", priority: ",
                       get_priority(),
                       ", max_flow_credits: ",
                       flow_control.get_max_credits(),
                       ", flow_credits: ",
                       flow_control.get_credits(),
                       ", subscheds size: ",
                       subscheds.size(),
                       "\n");

    std::stringstream msg;
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i]->dump(msg, i);
    }

    for (const auto& sched : subscheds) {
        sched->dump(out);
    }

    out << msg.str();

    ccl_logger::format(out, "--------------------------------\n");
}

ccl_sched::ccl_sched_ptr ccl_sched::create(const ccl_coll_param& param, const ccl_coll_attr& attr) {
    ccl_sched_key key;
    ccl_sched_ptr sched;
    bool is_created = false;
    auto create_fn = [param]() -> ccl_sched_ptr {
        return new ccl_sched({ ccl_sched_regular, param.comm->get_sched_id(false), param });
    };

    if (attr.to_cache) {
        key.set(param, attr);
        std::tie(sched, is_created) =
            ccl::global_data::get().sched_cache->find_or_create(std::move(key), create_fn);
    }
    else {
        sched = create_fn();
        is_created = true;
    }

    if (is_created) {
        sched->set_coll_attr(attr);
        sched->alloc_buffers_for_pre_post_copy();
        LOG_DEBUG("didn't find sched, create new one ",
                  sched,
                  ", coll ",
                  ccl_coll_type_to_str(sched->coll_param.ctype));
    }
    else {
        /* update some parameters and attributes in existing schedule
           as they could be changed since previous call */
        sched->update_coll_param_and_attr(param, attr);
        LOG_DEBUG(
            "found sched, reuse ", sched, ", type ", ccl_coll_type_to_str(sched->coll_param.ctype));
    }

    return sched;
}

void ccl_sched::do_progress() {
    for (auto entry_idx = start_idx; entry_idx < entries.size(); ++entry_idx) {
        auto& entry = entries[entry_idx];

        if (entry->get_status() == ccl_sched_entry_status_not_started) {
            LOG_DEBUG("starting entry: ",
                      entry.get(),
                      ", name: ",
                      entry->name(),
                      " [",
                      entry_idx,
                      "/",
                      entries.size(),
                      "]");
        }

        entry->do_progress();

        if (entry->get_status() == ccl_sched_entry_status_again) {
            LOG_DEBUG("entry ",
                      entry->name(),
                      " is in again state, stop progressing [",
                      entry_idx,
                      "/",
                      entries.size(),
                      "]");
            break;
        }

        if (entry_idx == start_idx && entry->is_completed()) {
            /* the entry has been completed, increment start_idx */
            ++start_idx;
            LOG_DEBUG("completed entry: ",
                      entry.get(),
                      ", name: ",
                      entry->name(),
                      entry->is_barrier() ? " barrier" : "",
                      " entry [",
                      entry_idx,
                      "/",
                      entries.size(),
                      "], shift start_idx to ",
                      start_idx,
                      ", sched ",
                      this);
        }
        else if (entry->is_barrier() && (!entry->is_completed() || (start_idx != entry_idx + 1))) {
            /* barrier is not completed or completed too early, skip the further progressing */
            break;
        }
    }
}

bool ccl_sched::is_strict_order_satisfied() {
    return std::all_of(entries.begin(), entries.end(), [](const sched_entry_ptr& e) {
        return e->is_strict_order_satisfied();
    });
}

void ccl_sched::complete_sched() {
    CCL_ASSERT(req, "ccl_sched must have req");

    if (ccl::global_data::env().sched_profile) {
        timer.stop();
        if (entries.size() > 0) {
            std::stringstream ss;
            ss << "\ncoll:";

            ccl_coll_param* profile_param = &(static_cast<ccl_sched*>(req)->coll_param);
            ss << ccl_coll_type_to_str(profile_param->ctype);

            /* TODO: tmp check, replace ccl_coll_entry_param by ccl_coll_param */
            if (!profile_param->send_counts.empty()) {
                ss << " count:" << profile_param->get_send_count();
            }

            ss << " time(usec):\ntotal: " << timer.str() << "\n";
            for (size_t idx = 0; idx < entries.size(); ++idx) {
                ss << "[" << idx << "] " << entries[idx]->name() << ": "
                   << entries[idx]->timer.str() << "\n";
            }
            ss << "-----------------------------";
            logger.info(ss.str());
        }
    }

    sched_complete_hook();

    // finish profile measurement only once per collective, so this is only
    // done for master sched that completes the last
    if (req->complete_request() && req == parent_sched) {
#ifdef CCL_ENABLE_ITT
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
        // only applicable for device execution
        if (coll_param.stream) {
            ccl::profile::itt::task_end(ccl::profile::itt::task_type::completion);
        }
#endif // CCL_ENABLE_SYCL
        ccl::profile::itt::task_end(ccl::profile::itt::task_type::operation);
#endif // CCL_ENABLE_ITT
    }
}

void ccl_sched::renew(bool need_update_id) {
    if (need_update_id) {
        update_id();
    }

    start_idx = 0;

    if (ccl::global_data::env().sched_profile) {
        timer.start();
    }

    for (size_t idx = 0; idx < entries.size(); idx++) {
        entries[idx].get()->reset(idx);
    }
}

void ccl_sched::add_barrier() {
    if (!entries.empty()) {
        if (add_mode == ccl_sched_add_back)
            entries.back()->make_barrier();
        else if (add_mode == ccl_sched_add_front)
            entries.front()->make_barrier();
        else
            CCL_FATAL("unexpected add_mode ", add_mode);
    }
}

std::vector<ccl::event>& ccl_sched::get_deps() const {
    return static_cast<ccl_sched*>(req)->coll_param.deps;
}

size_t ccl_sched::entries_count() const {
    return entries.size();
}
