#include <numeric>
#include <vector>
#include <iostream>

#include "oneapi/ccl.hpp"
#include "gtest/gtest.h"
#include "mpi.h"

class allgatherv_test : public ::testing::Test {
protected:
    void SetUp() override {
        ccl::init();

        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &size);
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }

    void TearDown() override {
        // don't do finalize if the case has failed, this
        // could lead to a deadlock due to inconsistent state.
        if (HasFatalFailure()) {
            return;
        }

        int is_finalized = 0;
        MPI_Finalized(&is_finalized);

        if (!is_finalized)
            MPI_Finalize();
    }

    int size;
    int rank;
};

TEST_F(allgatherv_test, allgatherv_vector) {
    const size_t count = 50;

    int i = 0;

    sycl::queue q;
    ASSERT_TRUE(q.get_device().is_gpu())
        << "test expects GPU device, please use SYCL_DEVICE_FILTER accordingly";

    /* create kvs */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    auto send_buf = sycl::malloc_shared<int>(count, q);
    std::vector<int *> recv_bufs;
    for (int i = 0; i < size; ++i) {
        recv_bufs.push_back(sycl::malloc_shared<int>(count, q));
    }

    sycl::buffer<int> expected_buf(count * size);
    sycl::buffer<int> check_buf(count * size);
    std::vector<size_t> recv_counts(size, count);

    std::vector<sycl::event> events;

    /* fill send buffer and check buffer */
    events.push_back(q.submit([&](auto &h) {
        sycl::accessor expected_buf_acc(expected_buf, h, sycl::write_only);
        h.parallel_for(count, [=, rnk = rank, sz = size](auto id) {
            send_buf[id] = rnk + id;
            for (int idx = 0; idx < sz; idx++) {
                expected_buf_acc[idx * count + id] = idx + id;
            }
        });
    }));

    for (i = 0; i < size; ++i) {
        events.push_back(q.submit([&](auto &h) {
            h.parallel_for(count, [=, rb = recv_bufs[i]](auto id) {
                rb[id] = -1;
            });
        }));
    }

    /* do not wait completion of kernel and provide it as dependency for operation */
    std::vector<ccl::event> deps;
    for (auto e : events) {
        deps.push_back(ccl::create_event(e));
    }

    /* invoke allgatherv */
    auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
    ccl::allgatherv(send_buf, count, recv_bufs, recv_counts, comm, stream, attr, deps).wait();

    /* open recv_buf and check its correctness on the device side */
    for (i = 0; i < size; ++i) {
        q.submit([&](auto &h) {
             sycl::accessor expected_buf_acc(expected_buf, h, sycl::read_only);
             sycl::accessor check_buf_acc(check_buf, h, sycl::write_only);
             h.parallel_for(count, [=, rb = recv_bufs[i]](auto id) {
                 if (rb[id] != expected_buf_acc[i * count + id]) {
                     check_buf_acc[i * count + id] = -1;
                 }
             });
         }).wait();
    }

    size_t total_recv = size * count;
    /* print out the result of the test on the host side */
    {
        sycl::host_accessor check_buf_acc(check_buf, sycl::read_only);
        for (i = 0; i < total_recv; i++) {
            if (check_buf_acc[i] == -1) {
                printf("unexpected value at idx %d\n", i);
                fflush(stdout);
            }
            ASSERT_NE(check_buf_acc[i], -1) << "check failed for receive buffer";
        }
    }

    return;
}
