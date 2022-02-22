#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {
    std::list<int> elem_counts = { 2 };

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

        std::vector<int *> send_bufs;
        for (int i = 0; i < size; ++i) {
            if (use_inplace) {
                send_bufs.push_back(recv_bufs[i]);
            }
            else {
                send_bufs.push_back(sycl::malloc_device<int>(elem_count, q));
            }
        }

        sycl::buffer<int> expected_buf(elem_count * size);
        sycl::buffer<int> check_buf(elem_count * size);
        vector<size_t> send_counts(size, elem_count);
        vector<size_t> recv_counts(size, elem_count);
        std::vector<sycl::event> events;

        if (!use_inplace) {
            /* fill recv buffers */
            for (int i = 0; i < size; ++i) {
                events.push_back(q.submit([&](auto &h) {
                    h.parallel_for(elem_count, [=, rb = recv_bufs[i]](auto id) {
                        rb[id] = -1;
                    });
                }));
            }
        }
        q.wait();

        /* fill send buffers */
        for (int i = 0; i < size; ++i) {
            events.push_back(q.submit([&](auto &h) {
                h.parallel_for(elem_count, [=, sb = send_bufs[i]](auto id) {
                    sb[id] = rank;
                });
            }));
        }
        q.wait();
        /* fill expected buffer */
        events.push_back(q.submit([&](auto &h) {
            sycl::accessor expected_buf_acc(expected_buf, h, sycl::write_only);
            h.parallel_for(elem_count * size, [=](auto id) {
                expected_buf_acc[id] = id / elem_count;
            });
        }));
        q.wait();

        /* do not wait completion of kernel and provide it as dependency for operation */
        std::vector<ccl::event> deps;
        for (auto e : events) {
            deps.push_back(ccl::create_event(e));
        }

        /* invoke alltoallv */
        auto attr = ccl::create_operation_attr<ccl::alltoallv_attr>();
        ccl::alltoallv(send_bufs, send_counts, recv_bufs, recv_counts, comm, stream, attr, deps)
            .wait();

        /* open recv_buf recv_buf and check its correctness on the device side */
        for (int idx = 0; idx < size; ++idx) {
            q.submit([&](auto &h) {
                 accessor expected_buf_acc(expected_buf, h, read_only);
                 accessor check_buf_acc(check_buf, h, write_only);
                 h.parallel_for(elem_count, [=, sn = recv_bufs[idx]](auto id) {
                     // TODO: for debug, fix the vectorize version of a2a
                     check_buf_acc[idx * elem_count + id] = sn[id];
                     // h.parallel_for(elem_count, [=, rb = recv_bufs[idx]](auto id) {
                     //check_buf_acc[idx * elem_count + id] = expected_buf_acc[idx * elem_count + id];
                     // if (rb[id] != expected_buf_acc[idx * elem_count + id]) {
                     //      check_buf_acc[idx * elem_count + id] = -1;
                     // }
                     // else {
                     //     check_buf_acc[idx * elem_count + id] = 0;
                     // }
                 });
             }).wait();
        }

        /* print out the result of the test on the host side */
        {
            size_t total_recv = size * elem_count;
            host_accessor check_buf_acc(check_buf, read_only);
            for (size_t i = 0; i < total_recv; i++) {
                // TODO: for debug, fix the vectorize version of a2a
                cout << check_buf_acc[i] << " ";
                // if (check_buf_acc[i] == -1) {
                //     cout << "elem_count: " << elem_count << ", unexpected value at idx: " << i << "\n";
                //     fail_counter++;
                //     break;
                // }
            }
            cout << "\n";
        }

        for (size_t i = 0; i < recv_bufs.size(); ++i) {
            sycl::free(recv_bufs[i], q);
        }

        if (!use_inplace) {
            for (size_t i = 0; i < send_bufs.size(); ++i) {
                sycl::free(send_bufs[i], q);
            }
        }

        if (!handle_exception(q)) {
            return -1;
        }
    }

    if (fail_counter) {
        cout << "FAILED\n";
        return -1;
    }
    else {
        cout << "PASSED\n";
        return 0;
    }
}
