#pragma once

#include "sched/sched.hpp"

class ccl_master_sched;

class ccl_extra_sched : public ccl_request, public ccl_sched {
public:
    static constexpr const char* class_name() {
        return "extra_sched";
    }

    ccl_extra_sched(const ccl_sched_create_param& param, ccl_master_sched* master_sched = nullptr)
            : ccl_request(),
              ccl_sched(param, this, master_sched) {
#ifdef ENABLE_DEBUG
        set_dump_callback([this](std::ostream& out) {
            dump(out);
        });
#endif
    }

    ~ccl_extra_sched() override = default;

    void dump(std::ostream& out) const;
};
