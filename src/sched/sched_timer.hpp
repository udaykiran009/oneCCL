#pragma once

#include <chrono>
#include <string>

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include <ze_api.h>
#endif

namespace ccl {

class sched_timer {
public:
    sched_timer() = default;
    void start() noexcept;
    void stop();
    std::string str() const;
    void print(std::string title = {}) const;
    void reset() noexcept;

private:
    long double time_usec;
    std::chrono::high_resolution_clock::time_point start_time{};

    long double get_time() const noexcept;
};

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
class kernel_timer {
public:
    kernel_timer();
    ~kernel_timer() = default;

    kernel_timer(const kernel_timer&) = default;

    void set_name(const std::string name);
    const std::string& get_name() const;

    void set_kernel_time(std::pair<uint64_t, uint64_t> val);
    void set_operation_event_time(std::pair<uint64_t, uint64_t> val);
    void set_operation_create_time(uint64_t val);
    void set_operation_start_time(uint64_t val);
    void set_operation_end_time(uint64_t val);
    void set_deps_start_time(uint64_t val);
    void set_deps_end_time(uint64_t val);
    void set_kernel_submit_time(uint64_t val);

    bool print(bool delay = true) const;
    void reset();

    static uint64_t get_current_time();

private:
    // Special pair of values that indicate unitialized measurements
    static constexpr std::pair<uint64_t, uint64_t> get_uninit_values() {
        return { std::numeric_limits<uint64_t>::max(), std::numeric_limits<uint64_t>::max() };
    }

    std::string name;
    // List of timestamps we're collecting
    std::pair<uint64_t, uint64_t> kernel_time;
    std::pair<uint64_t, uint64_t> operation_event_time;
    uint64_t operation_create_time;
    uint64_t operation_start_time;
    uint64_t operation_end_time;
    uint64_t deps_start_time;
    uint64_t deps_end_time;
    uint64_t kernel_submit_time;
};

class kernel_timer_printer {
public:
    void add_timer(const kernel_timer& timer) {
        timers.push_back(timer);
    }

    ~kernel_timer_printer() {
        for (auto& t : timers) {
            t.print(false);
        }
    }

private:
    std::vector<kernel_timer> timers;
};

#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

} //namespace ccl