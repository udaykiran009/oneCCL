#include "env.h"
#include "log.h"

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
