#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

void print_timings(
    size_t num_iters,
    size_t count,
    size_t compute_loop_count,
    const std::vector<std::tuple<sycl::event, sycl::event, sycl::event>> &kernel_events,
    const std::vector<size_t> &coll_timings) {
    auto convert_output = [](uint64_t val) {
        std::stringstream ss;
        ss << (val / 1000) << "." << (val % 1000) / 100;

        return ss.str();
    };

    auto get_exec_time = [](sycl::event e) {
        return e.get_profiling_info<sycl::info::event_profiling::command_end>() -
               e.get_profiling_info<sycl::info::event_profiling::command_start>();
    };

    for (size_t i = 0; i < num_iters; ++i) {
        auto fill_event = std::get<0>(kernel_events[i]);
        auto compute_event = std::get<1>(kernel_events[i]);
        auto check_event = std::get<2>(kernel_events[i]);

        std::cout << "iteration: " << i << ", count: " << count
                  << ", compute loop count: " << compute_loop_count
                  << ", time(usec): " << std::endl;

        std::cout << "   fill kernel: " << convert_output(get_exec_time(fill_event)) << std::endl;
        std::cout << "   compute kernel: " << convert_output(get_exec_time(compute_event))
                  << std::endl;
        std::cout << "   check kernel: " << convert_output(get_exec_time(check_event)) << std::endl;
        std::cout << "   allreduce call: " << convert_output(coll_timings[i]) << std::endl;
        std::cout << "\n" << std::endl;
    }
}

int main(int argc, char *argv[]) {
    size_t count = 10485760;

    int size = 0;
    int rank = 0;

    size_t compute_kernel_loop_count = 1;
    size_t num_iters = 10;

    if (argc > 1)
        num_iters = atoi(argv[1]);
    if (argc > 2)
        count = atoi(argv[2]);
    if (argc > 3)
        compute_kernel_loop_count = atoi(argv[3]);

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    cl::sycl::property_list props{ sycl::property::queue::in_order{},
                                   sycl::property::queue::enable_profiling{} };
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

    // Allocate enough memory to check results on each iteration without
    // additional memory transfer between host and device.
    sycl::buffer<int> check_buf{ count * num_iters };

    std::vector<ccl::event> ccl_events;
    std::vector<sycl::event> sycl_events;

    // Store allocated mem ptrs to free them later
    std::vector<std::pair<int *, int *>> ptrs(num_iters);
    // allocate all the buffers
    for (size_t i = 0; i < num_iters; ++i) {
        auto send_buf = allocator.allocate(count, usm::alloc::device);
        auto recv_buf = allocator.allocate(count, usm::alloc::device);
        ptrs[i] = { send_buf, recv_buf };
    }

    // compute multiplier for result checking
    size_t check_mult = 1;
    for (size_t j = 0; j < compute_kernel_loop_count; ++j) {
        check_mult *= 2;
    }

    std::vector<size_t> coll_timings(num_iters);
    std::vector<std::tuple<sycl::event, sycl::event, sycl::event>> kernel_events(num_iters);

    for (size_t i = 0; i < num_iters; ++i) {
        std::cout << "Running iteration " << i << std::endl;

        auto *send_buf = ptrs[i].first;
        auto *recv_buf = ptrs[i].second;

        /* open buffers and modify them on the device side */
        auto fill_event = q.submit([&](auto &h) {
            h.parallel_for(count, [=](auto id) {
                // Make the value differ on each iteration
                send_buf[id] = (i + 1) * (rank + 1);
                recv_buf[id] = -1;
            });
        });

        // Simple compute kernel
        auto compute_event = q.submit([&](auto &h) {
            h.parallel_for(count, [=](auto id) {
                for (size_t j = 0; j < compute_kernel_loop_count; ++j) {
                    send_buf[id] *= 2;
                }
            });
        });

        // We have to store the output event so it won't be destroyed
        // too early, although we don't have to do anything with it
        // as the test uses in-order queue and ccl already syncs it with
        // internal ones that run our kernels
        auto coll_start = std::chrono::high_resolution_clock::now();
        auto e = ccl::allreduce(send_buf, recv_buf, count, ccl::reduction::sum, comm, stream);
        auto coll_end = std::chrono::high_resolution_clock::now();
        ccl_events.push_back(std::move(e));
        coll_timings[i] =
            std::chrono::duration_cast<std::chrono::nanoseconds>(coll_end - coll_start).count();

        /* submit result checking after allreduce completion */
        auto check_event = q.submit([&](auto &h) {
            auto check_buf_acc = check_buf.get_access<sycl::access_mode::write>(h);
            h.parallel_for(count, [=](auto id) {
                if (recv_buf[id] !=
                    static_cast<int>(check_mult * (i + 1) * (size * (size + 1) / 2))) {
                    check_buf_acc[count * i + id] = -1;
                }
            });
        });
        sycl_events.push_back(check_event);

        kernel_events[i] = { fill_event, compute_event, check_event };
    }

    std::cout << "Submitted all the work" << std::endl;

    // Wait on the last generated events from each iteration
    for (auto &&e : sycl_events)
        e.wait();

    // Make sure there is no exceptions in the queue
    if (!handle_exception(q))
        return -1;

    for (auto p : ptrs) {
        allocator.deallocate(p.first);
        allocator.deallocate(p.second);
    }

    const char *print_var = std::getenv("CCL_REG_TEST_PRINT");
    if (print_var != nullptr) {
        bool should_print = false;
        if (strncmp(print_var, "all", 3) == 0)
            should_print = true;

        if (strncmp(print_var, "r", 1) == 0) {
            should_print = (rank == atoi(print_var + 1));
        }

        if (should_print) {
            std::cout << "Printing time info on " << rank << std::endl;
            print_timings(num_iters, count, compute_kernel_loop_count, kernel_events, coll_timings);
        }
    }

    /* Check if we have an error on some iteration */
    auto check_buf_acc = check_buf.get_access<sycl::access_mode::read>();
    {
        for (size_t i = 0; i < count * num_iters; i++) {
            if (check_buf_acc[i] == -1) {
                cout << "FAILED\n";
                return -1;
            }
        }
        cout << "PASSED\n";
    }

    return 0;
}
