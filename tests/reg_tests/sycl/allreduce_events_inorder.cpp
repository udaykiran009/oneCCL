#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {
    const size_t count = 10 * 1024 * 1024;

    int i = 0;
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    cl::sycl::property_list props{ cl::sycl::property::queue::in_order{} };
    queue q{ props };

    buf_allocator<int> allocator(q);

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

    /* create buffers */
    const int num_iters = 10;

    // Allocate enough memory to check results on each iteration without
    // additional memory transfer between host and device.
    int *check_buf = allocator.allocate(count * num_iters, usm::alloc::shared);

    std::vector<ccl::event> ccl_events;
    std::vector<sycl::event> sycl_events;

    // Store allocated mem ptrs to free them later
    std::vector<int *> ptrs;

    for (int i = 0; i < num_iters; ++i) {
        std::cout << "Running iteration " << i << std::endl;
        auto send_buf = allocator.allocate(count, usm::alloc::device);
        auto recv_buf = allocator.allocate(count, usm::alloc::device);

        ptrs.push_back(send_buf);
        ptrs.push_back(recv_buf);

        /* open buffers and modify them on the device side */
        q.submit([&](auto &h) {
            h.parallel_for(count, [=](auto id) {
                // Make the value differ on each iteration
                send_buf[id] = (i + 1) * (rank + 1);
                recv_buf[id] = -1;
            });
        });

        // We have to store the output event so it won't be destroyed
        // too early, although we don't have to do anything with it
        // as the test uses in-order queue and ccl already syncs it with
        // internal ones that run our kernels
        ccl_events.push_back(
            ccl::allreduce(send_buf, recv_buf, count, ccl::reduction::sum, comm, stream));

        /* submit result checking after allreduce completion */
        sycl_events.push_back(q.submit([&](auto &h) {
            h.parallel_for(count, [=](auto id) {
                if (recv_buf[id] != (i + 1) * (size * (size + 1) / 2)) {
                    check_buf[count * i + id] = -1;
                }
            });
        }));
    }

    // Wait on the last generated events from each iteration
    for (auto &&e : sycl_events)
        e.wait();

    for (int *ptr : ptrs) {
        allocator.deallocate(ptr);
    }

    // Make sure there is no exceptions in the queue
    if (!handle_exception(q))
        return -1;

    /* Check if we have an error on some iteration */
    {
        for (i = 0; i < count * num_iters; i++) {
            if (check_buf[i] == -1) {
                cout << "FAILED\n";
                return -1;
            }
        }
        if (i == count * num_iters) {
            cout << "PASSED\n";
        }
    }

    return 0;
}
