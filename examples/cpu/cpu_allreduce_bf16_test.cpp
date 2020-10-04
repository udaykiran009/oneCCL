#include <inttypes.h>
#include <iostream>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#include "bf16.hpp"
#include "oneapi/ccl.hpp"

#define COUNT (1048576 / 256)

#define CHECK_ERROR(send_buf, recv_buf, comm) \
    { \
        /* https://www.mcs.anl.gov/papers/P4093-0713_1.pdf */ \
        size_t comm_size = comm.size(); \
        double log_base2 = log(comm_size) / log(2); \
        double g = (log_base2 * BF16_PRECISION) / (1 - (log_base2 * BF16_PRECISION)); \
        for (size_t i = 0; i < COUNT; i++) { \
            double expected = ((comm_size * (comm_size - 1) / 2) + ((float)(i) * comm_size)); \
            double max_error = g * expected; \
            if (fabs(max_error) < fabs(expected - recv_buf[i])) { \
                printf( \
                    "[%zu] got recv_buf[%zu] = %0.7f, but expected = %0.7f, max_error = %0.16f\n", \
                    comm.rank(), \
                    i, \
                    recv_buf[i], \
                    (float)expected, \
                    (double)max_error); \
                return -1; \
            } \
        } \
    }

using namespace std;

int main() {

    const size_t count = 4096;

    size_t idx = 0;

    float send_buf[count];
    float recv_buf[count];

    short send_buf_bf16[count];
    short recv_buf_bf16[count];

    ccl::init();

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    auto comm = ccl::create_communicator(size, rank, kvs);

    for (idx = 0; idx < count; idx++) {
        send_buf[idx] = rank + idx;
        recv_buf[idx] = 0.0;
    }

    if (is_bf16_enabled() == 0) {
        cout << "WARNING: BF16 is disabled, skip test\n";
    }
    else {
        cout << "BF16 is enabled\n";
        convert_fp32_to_bf16_arrays(send_buf, send_buf_bf16, count);
        ccl::allreduce(send_buf_bf16,
                       recv_buf_bf16,
                       count,
                       ccl::datatype::bfloat16,
                       ccl::reduction::sum,
                       comm).wait();
        convert_bf16_to_fp32_arrays(recv_buf_bf16, recv_buf, count);
        CHECK_ERROR(send_buf, recv_buf, comm);

        if (rank == 0)
            cout << "PASSED\n";
    }

    MPI_Finalize();

    return 0;
}
