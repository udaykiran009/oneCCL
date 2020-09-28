#pragma once

#include "sched/sched.hpp"

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

    void dump(std::ostream& out) const;
};
