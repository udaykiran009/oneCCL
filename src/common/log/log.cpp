#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "common/log/log.hpp"

ccl_log_level ccl_logger::level = ccl_log_level::ERROR;
ccl_logger logger;

std::ostream& operator<<(std::ostream& os, ccl_streambuf& buf) {
    buf.set_eol();
    os << buf.buffer.get();
    buf.reset();
    return os;
}

void ccl_logger::write_prefix(std::ostream& str) {
    constexpr size_t time_buf_size = 20;
    time_t timer;
    char time_buf[time_buf_size]{};
    struct tm time_info {};
    time(&timer);
    if (localtime_r(&timer, &time_info)) {
        strftime(time_buf, time_buf_size, "%Y:%m:%d-%H:%M:%S", &time_info);
        str << time_buf;
    }
    str << ":(" << gettid() << ") ";
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
