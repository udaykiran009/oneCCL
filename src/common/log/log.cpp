#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common/log/log.hpp"

ccl_log_level ccl_logger::level = ccl_log_level::warn;
bool ccl_logger::abort_on_throw = false;
ccl_logger logger;

std::map<ccl_log_level, std::string> ccl_logger::level_names = {
    std::make_pair(ccl_log_level::error, "error"),
    std::make_pair(ccl_log_level::warn, "warn"),
    std::make_pair(ccl_log_level::info, "info"),
    std::make_pair(ccl_log_level::debug, "debug"),
    std::make_pair(ccl_log_level::trace, "trace")
};

std::ostream& operator<<(std::ostream& os, ccl_streambuf& buf) {
    buf.set_eol();
    os << buf.buffer.get();
    buf.reset();
    return os;
}

void ccl_logger::write_prefix(std::ostream& str) {
    constexpr size_t time_buf_size = 20;
    constexpr size_t tid_width = 5;
    time_t timer;
    char time_buf[time_buf_size]{};
    struct tm time_info;
    memset(&time_info, 0, sizeof(time_info));

    time(&timer);
    if (localtime_r(&timer, &time_info)) {
        strftime(time_buf, time_buf_size, "%Y:%m:%d-%H:%M:%S", &time_info);
        str << time_buf;
    }
    str << ":(" << std::setw(tid_width) << gettid() << ") ";
}

void ccl_logger::write_backtrace(std::ostream& str) {
    void* buffer[100];
    auto nptrs = backtrace(buffer, 100);
    str << "backtrace() returned " << nptrs << " addresses\n";

    auto strings = backtrace_symbols(buffer, nptrs);
    if (!strings) {
        str << "backtrace_symbols error\n";
        return;
    }

    for (int j = 0; j < nptrs; j++) {
        str << strings[j] << "\n";
    }
    free(strings);
}
