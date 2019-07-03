#include "common/log/log.hpp"
#include <execinfo.h>
#include <sys/syscall.h>
#include <unistd.h>

iccl_log_level iccl_logger::level = iccl_log_level::ERROR;
thread_local iccl_logger logger;

std::ostream& operator<<(std::ostream& os,
                         iccl_streambuf& buf)
{
    buf.set_eol();
    os << buf.buffer.get();
    buf.reset();
    return os;
}

void iccl_logger::write_prefix(std::ostream& str)
{
#if __GNUC__ >= 5
    std::time_t t = std::time(nullptr);
    std::tm tm = *std::localtime(&t);
    str << std::put_time(&tm, "%Y:%m:%d-%H:%M:%S");
#else
    constexpr size_t time_buf_size = 20;
    time_t timer;
    char time_buf[time_buf_size]{};
    struct tm* time_info = nullptr;
    time(&timer);
    time_info = localtime(&timer);
    if(time_info)
    {
        strftime(time_buf, time_buf_size, "%Y:%m:%d-%H:%M:%S", time_info);
        str << time_buf;
    }
#endif
    str << ":(" << syscall(SYS_gettid) << ") ";
}

void iccl_logger::write_backtrace(std::ostream& str)
{
    void* buffer[100];
    auto nptrs = backtrace(buffer, 100);
    str << "backtrace() returned " << nptrs << " addresses\n";

    auto strings = backtrace_symbols(buffer, nptrs);
    if (!strings)
    {
        str << "backtrace_symbols error\n";
        return;
    }

    for (int j = 0; j < nptrs; j++)
    {
        str << strings[j] << "\n";
    }
    free(strings);
}
