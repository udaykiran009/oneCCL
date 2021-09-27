#include <iomanip>
#include <numeric>
#include <sstream>

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

kernel_timer::kernel_timer()
        : kernel_time{ get_uninit_values() },
          operation_time{ get_uninit_values() } {}

// Returns true if we have all the necessary data to print
bool kernel_timer::print() const {
    auto is_value_set = [](std::pair<uint64_t, uint64_t> val) {
        return val != get_uninit_values();
    };

    // Convert value from ns to usec and format it.
    auto convert_output = [](uint64_t val) {
        std::stringstream ss;
        ss << (val / 1000) << "," << (val % 1000) / 100 << " usec";

        return ss.str();
    };

    // Make sure we have all the measurements
    if (is_value_set(kernel_time) && is_value_set(operation_time)) {
        std::stringstream ss;
        ss << "Running kernel from: " << name << std::endl;
        ss << "Kernel timestamps: " << kernel_time.first << ", " << kernel_time.second << std::endl;
        ss << "Operation timestamps: " << operation_time.first << ", " << operation_time.second
           << std::endl;
        ss << "Kernel running time: " << convert_output(kernel_time.second - kernel_time.first)
           << std::endl;
        ss << "Completion time: " << convert_output(operation_time.second - kernel_time.second)
           << std::endl;
        ss << std::endl;
        std::cout << ss.str();
        return true;
    }

    return false;
}

void kernel_timer::reset() {
    kernel_time = { get_uninit_values() };
    operation_time = { get_uninit_values() };
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

void kernel_timer::set_operation_time(std::pair<uint64_t, uint64_t> val) {
    operation_time = val;
}

std::pair<uint64_t, uint64_t> kernel_timer::get_kernel_time() const {
    return kernel_time;
}

std::pair<uint64_t, uint64_t> kernel_timer::get_operation_time() const {
    return operation_time;
}

} // namespace ccl