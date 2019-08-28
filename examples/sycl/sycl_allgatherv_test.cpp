#include <iostream>
#include <stdio.h>

#include <CL/sycl.hpp>
#include "ccl.h"

#define COUNT     (10 * 1024 * 1024)
#define COLL_ROOT (0)

using namespace std;
using namespace cl::sycl;
using namespace cl::sycl::access;

int main(int argc, char** argv)
{
    int i = 0;
    int j = 0;
    size_t size = 0;
    size_t rank = 0;
    size_t* recv_counts;

    cl::sycl::queue q;
    ccl_request_t request;
    ccl_stream_t stream;

    ccl_init();

    ccl_get_comm_rank(NULL, &rank);
    ccl_get_comm_size(NULL, &size);

    /* create SYCL stream */
    ccl_stream_create(ccl_stream_sycl, &q, &stream);

    cl::sycl::buffer<int, 1> sendbuf(COUNT);
    cl::sycl::buffer<int, 1> expected_buf(COUNT * size);
    cl::sycl::buffer<int, 1> recvbuf(size * COUNT);

    recv_counts = static_cast<size_t*>(malloc(size * sizeof(size_t)));

    for (size_t idx = 0; idx < size; idx++)
        recv_counts[idx] = COUNT;

    /* open sendbuf and initialize it on the CPU side */
    auto host_acc_sbuf = sendbuf.get_access<mode::write>();
    auto expected_acc_buf = expected_buf.get_access<mode::write>();

    for (i = 0; i < COUNT; i++) {
        host_acc_sbuf[i] = rank;
    }

    for (i = 0; i < size; i++) {
        for (j = 0; j < COUNT; j++) {
            expected_acc_buf[i*COUNT + j] = i + 1;
        }
    }

    /* open sendbuf and modify it on the target device side */
    q.submit([&](cl::sycl::handler& cgh) {
        auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
       });
    });

    /* invoke ccl_allgatherv on the CPU side */
    ccl_allgatherv(&sendbuf,
                   COUNT,
                   &recvbuf,
                   recv_counts,
                   ccl_dtype_int,
                   NULL, /* attr */
                   NULL, /* comm */
                   stream,
                   &request);

    ccl_wait(request);

    /* open sendbuf and check its correctness on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_rbuf = recvbuf.get_access<mode::write>(cgh);
        auto expected_acc_buf_dev = expected_buf.get_access<mode::read>(cgh);
        cgh.parallel_for<class allgatherv_test_rbuf_check>(range<1>{size * COUNT}, [=](item<1> id) {
            if (dev_acc_rbuf[id] != expected_acc_buf_dev[id]) {
                dev_acc_rbuf[id] = -1;
            }
        });
    });

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto host_acc_rbuf_new = recvbuf.get_access<mode::read>();
        for (i = 0; i < size * COUNT; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED" << std::endl;
                break;
            }
        }
        if (i == size * COUNT) {
            cout << "PASSED" << std::endl;
        }
    }

    ccl_stream_free(stream);

    ccl_finalize();

    return 0;
}
