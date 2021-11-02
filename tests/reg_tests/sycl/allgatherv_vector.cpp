#include <numeric>
#include <vector>
#include <iostream>
#include "oneapi/ccl.hpp"
#include "sycl_base.hpp"
#include "mpi.h"

int main(int argc, char *argv[]) {
    std::list<int> elem_counts = { 1, 17, 1024, 1025, 32768, 262145 };

    int size;
    int rank;
    int use_inplace = 0;

    int fail_counter = 0;

    if (argc > 1) {
        use_inplace = atoi(argv[1]);
    }

    cout << "use_inplace: " << use_inplace << "\n";

    sycl::queue q;
    if (!q.get_device().is_gpu()) {
        cout << "test expects GPU device, please use SYCL_DEVICE_FILTER accordingly";
        return -1;
    }

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    atexit(mpi_finalize);

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

    for (auto elem_count : elem_counts) {
        cout << "check elem_count: " << elem_count << "\n";

        /* create buffers */
        std::vector<int *> recv_bufs;
        for (int i = 0; i < size; ++i) {
            recv_bufs.push_back(sycl::malloc_device<int>(elem_count, q));
        }
        auto send_buf = (use_inplace) ? recv_bufs[rank] : sycl::malloc_device<int>(elem_count, q);

        sycl::buffer<int> expected_buf(elem_count * size);
        sycl::buffer<int> check_buf(elem_count * size);
        std::vector<size_t> recv_counts(size, elem_count);

        std::vector<sycl::event> events;

        /* fill recv buffers */
        for (int i = 0; i < size; ++i) {
            if (use_inplace && (i == rank)) {
                /* buffer will be filled with send values in separate kernel below */
                continue;
            }
            events.push_back(q.submit([&](auto &h) {
                h.parallel_for(elem_count, [=, rb = recv_bufs[i]](auto id) {
                    rb[id] = -1;
                });
            }));
        }

        /* fill send buffer and expected buffer */
        events.push_back(q.submit([&](auto &h) {
            sycl::accessor expected_buf_acc(expected_buf, h, sycl::write_only);
            h.parallel_for(elem_count, [=, rnk = rank, sz = size](auto id) {
                send_buf[id] = rnk + id;
                for (int idx = 0; idx < sz; idx++) {
                    expected_buf_acc[idx * elem_count + id] = idx + id;
                }
            });
        }));

        /* do not wait completion of kernels and provide them as dependency for operation */
        std::vector<ccl::event> deps;
        for (auto e : events) {
            deps.push_back(ccl::create_event(e));
        }

        /* invoke allgatherv */
        auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
        ccl::allgatherv(send_buf, elem_count, recv_bufs, recv_counts, comm, stream, attr, deps)
            .wait();

        /* open recv_buf and check its correctness on the device side */
        for (int i = 0; i < size; ++i) {
            q.submit([&](auto &h) {
                 sycl::accessor expected_buf_acc(expected_buf, h, sycl::read_only);
                 sycl::accessor check_buf_acc(check_buf, h, sycl::write_only);
                 h.parallel_for(elem_count, [=, rb = recv_bufs[i]](auto id) {
                     if (rb[id] != expected_buf_acc[i * elem_count + id]) {
                         check_buf_acc[i * elem_count + id] = -1;
                     }
                     else {
                         check_buf_acc[i * elem_count + id] = 0;
                     }
                 });
             }).wait();
        }

        /* print out the result of the test on the host side */
        {
            size_t total_recv = size * elem_count;
            sycl::host_accessor check_buf_acc(check_buf, sycl::read_only);
            for (size_t i = 0; i < total_recv; i++) {
                if (check_buf_acc[i] == -1) {
                    fail_counter++;
                    cout << "elem_count: " << elem_count << ", unexpected value at idx: " << i
                         << "\n";
                    break;
                }
            }
        }

        for (size_t i = 0; i < recv_bufs.size(); ++i) {
            sycl::free(recv_bufs[i], q);
        }

        if (!use_inplace) {
            sycl::free(send_buf, q);
        }

        if (!handle_exception(q)) {
            return -1;
        }
    } // for elem_counts

    if (fail_counter) {
        cout << "FAILED\n";
    }
    else {
        cout << "PASSED\n";
    }

    return 0;
}
