#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

#include "uuid.h"

char* get_uuid()
{
    char uuid[UUID_STR_LEN] = {0};

    int pid;
    long hostid;

    struct timeval tv;
    gettimeofday(&tv, NULL);
    long long ticks = (long long)tv.tv_sec * 1000000 + (long long)tv.tv_usec;
    hostid = gethostid();
    pid = getpid();

    snprintf(uuid, UUID_STR_LEN,
             "%02hhx%02hhx%02hhx%02hhx-"
             "%02hhx%02hhx-%02hhx%02hhx-%02hhx%02hhx-"
             "%02hhx%02hhx%02hhx%02hhx%02hhx%02hhx",
             (short int)(pid), (short int)(pid >> 8), (short int)(pid >> 16), (short int)(pid >> 24),
             (short int)(ticks),        (short int)(ticks >> 8),
             (short int)(ticks >> 16),  (short int)(ticks >> 24),
             (short int)(ticks >> 32),  (short int)(ticks >> 40),
             (short int)(ticks >> 48),  (short int)(ticks >> 56),
             (short int)(hostid),       (short int)(hostid >> 8),
             (short int)(hostid >> 16), (short int)(hostid >> 24));

   return strdup(uuid);
}

