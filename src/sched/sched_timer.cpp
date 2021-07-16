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

} // namespace ccl
