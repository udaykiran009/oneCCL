#include "common/log/log.hpp"
#include "common/env/env.hpp"

void mlsl_log_get_time(char* buf, size_t buf_size)
{
    time_t timer;
    struct tm* time_info = 0;
    time(&timer);
    time_info = localtime(&timer);
    assert(time_info);
    strftime(buf, buf_size, "%Y:%m:%d %H:%M:%S", time_info);
}

void mlsl_log_print_backtrace(void)
{
    int j, nptrs;
    void* buffer[100];
    char** strings;

    nptrs = backtrace(buffer, 100);
    printf("backtrace() returned %d addresses\n", nptrs);
    fflush(stdout);

    strings = backtrace_symbols(buffer, nptrs);
    if (strings == NULL)
    {
        perror("backtrace_symbols");
        exit(EXIT_FAILURE);
    }

    for (j = 0; j < nptrs; j++)
    {
        printf("%s\n", strings[j]);
        fflush(stdout);
    }
    free(strings);
}

void mlsl_log_print_buffer(void* buf, size_t cnt, mlsl_datatype_internal_t dtype, const char* prefix)
{
    char b[1024];
    char* dump_buf = b;
    auto bytes_written = sprintf(dump_buf, "%s: cnt %zu, dt %s [",
                                 prefix, cnt, mlsl_datatype_get_name(dtype));
    dump_buf = (char*)dump_buf + bytes_written;
    size_t idx;
    for (idx = 0; idx < cnt; idx++)
    {
        if (dtype == mlsl_dtype_internal_float)
            bytes_written = sprintf(dump_buf, "%f ", ((float*)buf)[idx]);
        else
            MLSL_FATAL("unexpected");
        dump_buf += bytes_written;
    }
    bytes_written = sprintf(dump_buf, "]");
    MLSL_LOG(INFO, "%s", b);
}
