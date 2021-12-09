#include "common/global/global.hpp"
#include "common/utils/sync_object.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/extra_sched.hpp"
#include "sched/queue/queue.hpp"
#include "sched/sched.hpp"

ccl_sched::ccl_sched(const ccl_sched_create_param& param,
                     ccl_request* master_request,
                     ccl_master_sched* master_sched)
        : ccl_sched_base(param),
          req(master_request),
          master_sched(master_sched) {
    strict_order = ccl::global_data::env().enable_strict_order;
}

ccl_sched::~ccl_sched() {
    if (in_bin_status == ccl_sched_in_bin_added)
        LOG_DEBUG("in_bin_status == ccl_sched_in_bin_added");

    if (finalize_fn) {
        finalize_fn(this, finalize_fn_ctx);
    }
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

void ccl_sched::complete() {
    CCL_ASSERT(req, "ccl_sched must have req");

    if (ccl::global_data::env().sched_profile) {
        timer.stop();
        if (entries.size() > 0) {
            std::stringstream ss;
            ss << "\ncoll:";

            ccl_coll_param* profile_param = &(static_cast<ccl_master_sched*>(req)->coll_param);
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
    if (req->complete() && req == master_sched) {
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
    return static_cast<ccl_master_sched*>(req)->coll_param.deps;
}

void ccl_sched::dump(std::ostream& out) const {
    if (!ccl::global_data::env().sched_dump) {
        return;
    }

    ccl_sched_base::dump(out, class_name());
    ccl_logger::format(out,
                       ", start_idx: ",
                       start_idx,
                       ", num_entries: ",
                       entries.size(),
                       ", priority: ",
                       get_priority(),
                       ", max_flow_credits: ",
                       flow_control.get_max_credits(),
                       ", flow_credits: ",
                       flow_control.get_credits(),
                       "\n");

    std::stringstream msg;
    for (size_t i = 0; i < entries.size(); ++i) {
        entries[i]->dump(msg, i);
    }
    out << msg.str();
    ccl_logger::format(out, "--------------------------------\n");
}

size_t ccl_sched::entries_count() const {
    return entries.size();
}

ccl_comm_id_t ccl_sched::get_comm_id() {
    return coll_param.comm->id();
}
