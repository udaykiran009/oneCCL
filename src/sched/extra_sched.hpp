#pragma once

#include "sched/sched.hpp"
#include "common/global/global.hpp"

class ccl_extra_sched : public ccl_request, public ccl_sched {
public:
    static constexpr const char* class_name() {
        return "extra_sched";
    }

    ccl_extra_sched(ccl_coll_param& coll_param, ccl_sched_id_t id)
            : ccl_request(),
              ccl_sched(coll_param, this) {
        sched_id = id;
#ifdef ENABLE_DEBUG
        set_dump_callback([this](std::ostream& out) {
            dump(out);
        });
#endif
    }

    ~ccl_extra_sched() override = default;

    void dump(std::ostream& out) const {
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
                           "\n");
        std::stringstream msg;
        for (size_t i = 0; i < entries.size(); ++i) {
            entries[i]->dump(msg, i);
        }
        out << msg.str();
#ifdef ENABLE_TIMERS
        ccl_logger::format(out,
                           "\nlife time [us] ",
                           std::setw(5),
                           std::setbase(10),
                           std::chrono::duration_cast<std::chrono::microseconds>(
                               exec_complete_time - exec_start_time)
                               .count(),
                           "\n");
#endif

        ccl_logger::format(out, "--------------------------------\n");
    }
};
