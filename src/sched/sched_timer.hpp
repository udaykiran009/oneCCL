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

    long double get_elapsed_usec() const noexcept;

private:
    long double time_usec;
    std::chrono::high_resolution_clock::time_point start_time{};
};

#ifdef CCL_ENABLE_ITT

namespace profile {
namespace itt {

void set_thread_name(const std::string& name);

enum class task_type : int {
    operation = 0,
    api_call = 1,
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    preparation = 2,
    device_work = 3,
    deps_handling = 4,
    completion = 5
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
};

void task_start(task_type type);
void task_end(task_type type);

} // namespace itt
} // namespace profile

#endif // CCL_ENABLE_ITT

} //namespace ccl