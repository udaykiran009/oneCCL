#include <inttypes.h>
#include <iostream>
#include <math.h>
#include <mpi.h>
#include <stdio.h>
#include <string.h>

#include "base.hpp"
#include "bf16.hpp"
#include "oneapi/ccl.hpp"
#include "sycl_base.hpp"
#include "utils.hpp"

#define COUNT (1048576 / 256)

using namespace std;

int main() {
    const size_t count = COUNT;

    size_t idx = 0;

    float send_buf[count];
    float recv_buf[count];

    short send_buf_bf16[count];
    short recv_buf_bf16[count];

    const size_t bf16_buf_size = sizeof(short) * count;

    ccl::init();

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    sycl::queue q;
    if (!create_sycl_queue("gpu", rank, q)) {
        return -1;
    }

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

    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);
    /* create stream */
    auto stream = ccl::create_stream(q);

    for (idx = 0; idx < count; idx++) {
        send_buf[idx] = rank + idx;
        recv_buf[idx] = 0.0;
    }

    convert_fp32_to_bf16_arrays_generic(send_buf, send_buf_bf16, count);

    void* send_buf_bf16_device = malloc_device(bf16_buf_size, q);
    void* recv_buf_bf16_device = malloc_device(bf16_buf_size, q);

    // copy our bf16 buffer to device memory
    q.memcpy(send_buf_bf16_device, send_buf_bf16, bf16_buf_size).wait();

    ccl::allreduce(send_buf_bf16_device,
                   recv_buf_bf16_device,
                   count,
                   ccl::datatype::bfloat16,
                   ccl::reduction::sum,
                   comm,
                   stream)
        .wait();

    q.memcpy(recv_buf_bf16, recv_buf_bf16_device, bf16_buf_size).wait();

    convert_bf16_to_fp32_arrays_generic(recv_buf_bf16, recv_buf, count);

    CHECK_BF16_ERROR(send_buf, recv_buf, comm);

    cout << "PASSED\n";

    return 0;
}
