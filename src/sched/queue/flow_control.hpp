#pragma once

namespace ccl {

#define CCL_MAX_FLOW_CREDITS 1024

class flow_control {
public:
    flow_control();
    ~flow_control();

    void set_max_credits(size_t value);
    size_t get_max_credits() const;
    size_t get_credits() const;
    bool take_credit();
    void return_credit();

private:
    size_t max_credits;
    size_t min_credits;
    size_t credits;
};

} // namespace ccl
