#include <iomanip>
#include <numeric>
#include <sstream>

#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched_timer.hpp"

namespace ccl {

void sched_timer::start() noexcept {
    start_time = std::chrono::high_resolution_clock::now();
}

void sched_timer::stop() {
    auto stop_time = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::micro> time_span = stop_time - start_time;
    time_usec = time_span.count();
}

std::string sched_timer::str() const {
    std::stringstream ss;
    ss.precision(2);
    ss << std::fixed << get_time();
    return ss.str();
}

void sched_timer::print(std::string title) const {
    logger.info(title, ": ", this->str());
}

void sched_timer::reset() noexcept {
    time_usec = 0;
}

long double sched_timer::get_time() const noexcept {
    return time_usec;
}

#ifdef CCL_ENABLE_SYCL
kernel_timer::kernel_timer()
        : kernel_time{ get_uninit_values() },
          operation_event_time{ get_uninit_values() },
          operation_start_time{ std::numeric_limits<uint64_t>::max() },
          operation_end_time{ std::numeric_limits<uint64_t>::max() },
          kernel_submit_time{ std::numeric_limits<uint64_t>::max() } {}

// Returns true if we have all the necessary data to print
bool kernel_timer::print() const {
    auto is_value_set = [](std::pair<uint64_t, uint64_t> val) {
        return val != get_uninit_values();
    };

    auto is_val_set = [](uint64_t val) {
        return val != std::numeric_limits<uint64_t>::max();
    };

    // Convert value from ns to usec and format it.
    auto convert_output = [](uint64_t val) {
        std::stringstream ss;
        ss << (val / 1000) << "." << (val % 1000) / 100;

        return ss.str();
    };

    bool something_printed = false;

    std::stringstream ss;

    // currently there are 4 levels:
    // 0 - profiling and output is disabled
    // 1 - main measurements on device side(i.e. kernel time, completion time, etc.)
    // 2 - level 1 + host side measurements measurements(i.e. collective time)
    // 3 - level 1 + level 2 + raw timestamp that we collected
    int profile_level = ccl::global_data::env().enable_kernel_profile;

    bool all_measurements_are_ready =
        is_value_set(kernel_time) && is_value_set(operation_event_time) &&
        is_val_set(operation_start_time) && is_val_set(operation_end_time) &&
        is_val_set(kernel_submit_time);

    // Make sure we have all the measurements
    if (all_measurements_are_ready) {
        ss << "kernel: " << name << " time(usec)" << std::endl;
        if (profile_level > 2) {
            ss << "kernel timestamps: " << kernel_time.first << ", " << kernel_time.second
               << std::endl;
            ss << "kernel submited at: " << kernel_submit_time << std::endl;
            ss << "operation event timestamps: " << operation_event_time.first << ", "
               << operation_event_time.second << std::endl;
            ss << "operation start/end timestamp: " << operation_start_time << ", "
               << operation_end_time << std::endl;
        }

        if (profile_level > 1) {
            ss << "operation: " << convert_output(operation_end_time - operation_start_time)
               << std::endl;
            ss << "preparation: " << convert_output(kernel_submit_time - operation_start_time)
               << std::endl;
        }

        ss << "kernel: " << convert_output(kernel_time.second - kernel_time.first) << std::endl;
        ss << "completion: " << convert_output(operation_event_time.second - kernel_time.second)
           << std::endl;

        something_printed = true;
    }

    if (something_printed) {
        ss << std::endl;
        std::cout << ss.str();
    }

    return something_printed;
}

void kernel_timer::reset() {
    kernel_time = { get_uninit_values() };
    operation_event_time = { get_uninit_values() };
    operation_start_time = std::numeric_limits<uint64_t>::max();
    operation_end_time = std::numeric_limits<uint64_t>::max();
    kernel_submit_time = std::numeric_limits<uint64_t>::max();
}

uint64_t kernel_timer::get_current_time() {
    return std::chrono::time_point_cast<std::chrono::nanoseconds>(
               std::chrono::high_resolution_clock::now())
        .time_since_epoch()
        .count();
}

void kernel_timer::set_name(const std::string new_name) {
    name = new_name;
}
const std::string& kernel_timer::get_name() const {
    return name;
}

void kernel_timer::set_kernel_time(std::pair<uint64_t, uint64_t> val) {
    kernel_time = val;
}

void kernel_timer::set_operation_event_time(std::pair<uint64_t, uint64_t> val) {
    operation_event_time = val;
}

void kernel_timer::set_operation_start_time(uint64_t val) {
    operation_start_time = val;
}

void kernel_timer::set_operation_end_time(uint64_t val) {
    operation_end_time = val;
}

void kernel_timer::set_kernel_submit_time(uint64_t val) {
    kernel_submit_time = val;
}

std::pair<uint64_t, uint64_t> kernel_timer::get_kernel_time() const {
    return kernel_time;
}

std::pair<uint64_t, uint64_t> kernel_timer::get_operation_event_time() const {
    return operation_event_time;
}

uint64_t kernel_timer::get_operation_start_time() const {
    return operation_start_time;
}

uint64_t kernel_timer::get_operation_end_time() const {
    return operation_end_time;
}

uint64_t kernel_timer::get_kernel_submit_time() const {
    return kernel_submit_time;
}
#endif // CCL_ENABLE_SYCL

} // namespace ccl