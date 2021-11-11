#pragma once

#define CHECK_BF16_ERROR(send_buf, recv_buf, comm) \
    { \
        /* https://www.mcs.anl.gov/papers/P4093-0713_1.pdf */ \
        int comm_size = comm.size(); \
        double log_base2 = log(comm_size) / log(2); \
        double g = (log_base2 * BF16_PRECISION) / (1 - (log_base2 * BF16_PRECISION)); \
        for (size_t i = 0; i < COUNT; i++) { \
            double expected = ((comm_size * (comm_size - 1) / 2) + ((float)(i)*comm_size)); \
            double max_error = g * expected; \
            if (fabs(max_error) < fabs(expected - recv_buf[i])) { \
                printf( \
                    "[%d] got recv_buf[%zu] = %0.7f, but expected = %0.7f, max_error = %0.16f\n", \
                    comm.rank(), \
                    i, \
                    recv_buf[i], \
                    (float)expected, \
                    (double)max_error); \
                return -1; \
            } \
        } \
    }
