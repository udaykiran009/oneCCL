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
    size_t size = 0;
    size_t rank = 0;

    cl::sycl::queue q;
    ccl_request_t request;
    ccl_stream_t stream;

    ccl_init();

    ccl_get_comm_rank(NULL, &rank);
    ccl_get_comm_size(NULL, &size);

    /* create SYCL stream */
    ccl_stream_create(ccl_stream_sycl, &q, &stream);

    cl::sycl::buffer<int, 1> buf(COUNT);

    /* open buf and initialize it on the CPU side */
    auto host_acc_buf = buf.get_access<mode::write>();

    for (i = 0; i < COUNT; i++) {
        host_acc_buf[i] = rank;
    }

    /* open buf and modify it on the target device side */
    q.submit([&](cl::sycl::handler& cgh) {
        auto dev_acc_buf = buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_buf[id] += 1;
        });
    });

    /* invoke ccl_bcast on the CPU side */
    ccl_bcast(&buf,
              COUNT,
              ccl_dtype_int,
              COLL_ROOT,
              NULL,
              NULL,
              stream,
              &request);

    ccl_wait(request);

    /* open buf and check its correctness on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_buf = buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class bcast_test_rbuf_check>(range<1>{COUNT}, [=](item<1> id) {
            if (dev_acc_buf[id] != COLL_ROOT + 1) {
                dev_acc_buf[id] = -1;
            }
        });
    });

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto host_acc_buf_new = buf.get_access<mode::read>();
        for (i = 0; i < COUNT; i++) {
            if (host_acc_buf_new[i] == -1) {
                cout << "FAILED" << endl;
                break;
            }
        }
        if (i == COUNT) {
            cout << "PASSED" << endl;
        }
    }

    ccl_stream_free(stream);

    ccl_finalize();

    return 0;
}
