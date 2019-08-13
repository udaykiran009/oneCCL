#include <iostream>
#include <stdio.h>

#include <CL/sycl.hpp>
#include "ccl.hpp"

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
    ccl::environment env;
    ccl::communicator comm;

    rank = comm.rank();
    size = comm.size();

    /* create SYCL stream */
    ccl::stream stream(ccl::stream_type::sycl, &q);

    cl::sycl::buffer<int, 1> buf(COUNT);

    /* open buf and initialize it on the CPU side */
    auto host_acc_sbuf = buf.get_access<mode::write>();
    for (i = 0; i < COUNT; i++) {
        if (rank == COLL_ROOT)
            host_acc_sbuf[i] = rank;
        else
            host_acc_sbuf[i] = 0;
    }

    /* open buf and modify it on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_sbuf = buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class bcast_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
        });
    });

    /* invoke ccl_bcast on the CPU side */
    comm.bcast(&buf,
               COUNT,
               ccl::data_type::dt_int,
               COLL_ROOT,
               nullptr, /* attr */
               &stream)->wait();

    /* open buf and check its correctness on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_rbuf = buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class bcast_test_rbuf_check>(range<1>{COUNT}, [=](item<1> id) {
            if (dev_acc_rbuf[id] != COLL_ROOT + 1) {
                dev_acc_rbuf[id] = -1;
           }
       });
    });

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto host_acc_rbuf_new = buf.get_access<mode::read>();
        for (i = 0; i < COUNT; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED" << endl;
                break;
            }
        }
        if (i == COUNT) {
            cout << "PASSED" << endl;
        }
    }

    return 0;
}
