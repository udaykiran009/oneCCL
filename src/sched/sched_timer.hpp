#pragma once

#include <chrono>
#include <string>

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

class kernel_timer {
public:
    kernel_timer();
    ~kernel_timer() = default;

    void set_name(const std::string name);
    const std::string& get_name() const;

    void set_kernel_time(std::pair<uint64_t, uint64_t> val);
    void set_operation_event_time(std::pair<uint64_t, uint64_t> val);
    void set_operation_start_time(uint64_t val);
    void set_operation_end_time(uint64_t val);
    void set_kernel_submit_time(uint64_t val);
    std::pair<uint64_t, uint64_t> get_kernel_time() const;
    std::pair<uint64_t, uint64_t> get_operation_event_time() const;
    uint64_t get_operation_start_time() const;
    uint64_t get_operation_end_time() const;
    uint64_t get_kernel_submit_time() const;

    bool print() const;
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
    uint64_t operation_start_time;
    uint64_t operation_end_time;
    uint64_t kernel_submit_time;
};

} //namespace ccl