#include "sycl_base.hpp"

#include <numeric>

using namespace std;
using namespace sycl;

// TODO: there seems to be an overflow when calculating the stats on large number of iterations,
// think how we can fix this.
void print_times(size_t rank,
                 size_t iter_count,
                 size_t kernel_count,
                 const std::vector<std::tuple<size_t, size_t, size_t>> &kernel_events_times,
                 const std::vector<size_t> &allreduce_api_times) {
    auto convert_output = [](uint64_t val) {
        std::stringstream ss;
        ss << (val / 1000) << "." << (val % 1000) / 100;
        return ss.str();
    };

    // sum of additional time introduced by allreduce calls
    std::vector<size_t> allreduce_times;

    // number of iterations to warm up which we don't print
    size_t warm_up_iters = std::min(iter_count, std::max(iter_count / 10, 4ul));
    for (size_t i = 0; i < iter_count; ++i) {
        if (i < warm_up_iters) {
            continue;
        }

        std::stringstream ss;
        ss << "Rank: " << rank << ", Iteration: " << i << std::endl;
        for (size_t j = 0; j < kernel_count; ++j) {
            const auto &ktime = kernel_events_times[i * kernel_count + j];
            ss << "FWK submit kernel: " << j << " execution time "
               << convert_output(std::get<0>(ktime)) << " us " << std::endl;
            ss << "FWK update kernel: " << j << " execution time "
               << convert_output(std::get<1>(ktime)) << " us " << std::endl;

            size_t allreduce_time = std::get<2>(ktime);
            allreduce_times.push_back(allreduce_time);

            ss << "Time between FWK kernels(additional time introduced by CCL AllReduce): "
               << convert_output(allreduce_time) << " us " << std::endl;
            ss << "CCL Allreduce API call: " << convert_output(allreduce_api_times[j]) << " us "
               << std::endl;
        }

        if (getenv("VERBOSE_OUTPUT") != nullptr)
            std::cout << ss.str() << "\n" << std::endl;
    }

    if (allreduce_times.size() < 2)
        return;

    size_t total_allreduce_time =
        std::accumulate(allreduce_times.begin(), allreduce_times.end(), 0);

    std::cout << "Additional time introduced by CCL AllReduce: \n";
    std::cout << "Avg: " << convert_output(total_allreduce_time / allreduce_times.size()) << " us "
              << std::endl;
    std::sort(allreduce_times.begin(), allreduce_times.end());

    std::cout << "Min: " << convert_output(allreduce_times[0]) << " us " << std::endl;
    std::cout << "Max: " << convert_output(allreduce_times[allreduce_times.size() - 1]) << " us "
              << std::endl;
    std::cout << "Median: " << convert_output(allreduce_times[allreduce_times.size() / 2]) << " us "
              << std::endl;

    // calculate avg ignoring top 10%
    auto part_end = allreduce_times.end() - (allreduce_times.size() / 10);
    size_t part_size = part_end - allreduce_times.begin();
    size_t total_allreduce_part_times = std::accumulate(allreduce_times.begin(), part_end, 0);
    std::cout << "Avg(90%): " << convert_output(total_allreduce_part_times / part_size) << " us "
              << std::endl;
}

void append_kernel_times(std::vector<std::tuple<size_t, size_t, size_t>> &kernel_times,
                         const std::vector<std::tuple<sycl::event, sycl::event>> &kernel_events) {
    // kernel execution
    auto get_exec_time = [](sycl::event e) {
        return e.get_profiling_info<sycl::info::event_profiling::command_end>() -
               e.get_profiling_info<sycl::info::event_profiling::command_start>();
    };

    // kernel gap
    auto get_between_kernels_exec_time = [](sycl::event prev, sycl::event next) {
        return next.get_profiling_info<sycl::info::event_profiling::command_start>() -
               prev.get_profiling_info<sycl::info::event_profiling::command_end>();
    };

    for (size_t i = 0; i < kernel_events.size(); ++i) {
        auto prev = std::get<0>(kernel_events[i]);
        auto cur = std::get<1>(kernel_events[i]);

        size_t submit_kernel_time = get_exec_time(prev);
        size_t update_kernel_time = get_exec_time(cur);

        size_t allreduce_time = get_between_kernels_exec_time(prev, cur);

        kernel_times.push_back({ submit_kernel_time, update_kernel_time, allreduce_time });
    }
}

int main(int argc, char *argv[]) {
    size_t count = 2 * 1024 * 1024; // 8mb of floats

    int size = 0;
    int rank = 0;

    size_t iter_count = 20;
    size_t kernel_count = 15;

    if (argc > 1)
        kernel_count = atoi(argv[1]);
    if (argc > 2)
        count = atoi(argv[2]);
    if (argc > 3)
        iter_count = atoi(argv[3]);

    size_t byte_count = count * 4;

    ccl::init();
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    sycl::property_list props{ sycl::property::queue::in_order{},
                               sycl::property::queue::enable_profiling{} };
    queue q{ props };

    buf_allocator<float> allocator(q);

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

    // store allocated mem ptrs to free them later
    std::vector<std::pair<float *, float *>> ptrs(kernel_count);
    // allocate all the buffers
    for (size_t i = 0; i < kernel_count; i++) {
        float *weight_buf = allocator.allocate(byte_count, usm::alloc::device);
        float *weight_allreduce_buf = allocator.allocate(byte_count, usm::alloc::device);
        ptrs[i] = { weight_buf, weight_allreduce_buf };
    }

    std::vector<ccl::event> ccl_events;
    std::vector<std::tuple<sycl::event, sycl::event>> kernel_events;
    std::vector<std::tuple<size_t, size_t, size_t>> kernel_event_times;
    std::vector<size_t> allreduce_api_times(iter_count * kernel_count);

    const char *disable_var = std::getenv("DISABLE_CCL_ALLREDUCE");
    for (size_t i = 0; i < iter_count; ++i) {
        std::cout << "Running iteration " << i << std::endl;

        for (size_t j = 0; j < kernel_count; j++) {
            size_t num = i * kernel_count + j;
            float *weight_buf = ptrs[j].first;
            float *weight_allreduce_buf = ptrs[j].second;

            // step1: FWK kernel submission
            sycl::event submit_event;
            if (i == 0) {
                submit_event = q.submit([&](auto &h) {
                    h.parallel_for(count, [=](auto id) {
                        // initial weight in first iteration
                        weight_buf[id] = j * (rank + 1);
                    });
                });
            }
            else {
                submit_event = q.submit([&](auto &h) {
                    h.parallel_for(count, [=](auto id) {
                        // make weight differ in each iteration
                        weight_buf[id] = weight_buf[id] + (j * (rank + 1));
                    });
                });
            }

            // step2: Call CCL AllReduce API
            if (disable_var == nullptr) {
                // we have to store the output event so it won't be destroyed
                // too early, although we don't have to do anything with it
                // as the test uses in-order queue and ccl already syncs it with
                // internal ones that run our kernels
                auto coll_start = std::chrono::high_resolution_clock::now();
                auto ccl_event = ccl::allreduce(
                    weight_buf, weight_allreduce_buf, count, ccl::reduction::sum, comm, stream);
                auto coll_end = std::chrono::high_resolution_clock::now();
                ccl_events.push_back(std::move(ccl_event));
                allreduce_api_times[num] =
                    std::chrono::duration_cast<std::chrono::nanoseconds>(coll_end - coll_start)
                        .count();
            }

            // step3: Weight update
            auto update_event = q.submit([&](auto &h) {
                h.parallel_for(count, [=](auto id) {
                    // update weight in each iteration
                    weight_buf[id] = weight_allreduce_buf[id] * 0.5;
                });
            });

            kernel_events.push_back({ submit_event, update_event });
        }
        q.wait();

        // once we waited for all submitted kernels and allreduce ops, we can safely
        // destroy ccl events in order to clear underlying resources
        ccl_events.clear();

        // we can't simply keep the events since they use resources which results in
        // issues on large number of iterations(e.g. fd leaks). To avoid that, on each
        // iteration get their timings and destroy them.
        append_kernel_times(kernel_event_times, kernel_events);
        kernel_events.clear();
    }

    // when using CCL AllReduce, check weight accuracy after several iterations
    if (disable_var == nullptr) {
        auto weight_host_buf = allocator.allocate(byte_count, usm::alloc::host);
        q.memcpy(weight_host_buf, ptrs[kernel_count - 1].first, byte_count);
        q.wait();

        for (size_t n = 0; n < count; n++) {
            if (weight_host_buf[n] != (kernel_count - 1) * iter_count * size * (size + 1) / 4) {
                std::cout << "FAILED\n" << std::endl;
                return -1;
            }
        }
        std::cout << "PASSED\n" << std::endl;
    }

    print_times(rank, iter_count, kernel_count, kernel_event_times, allreduce_api_times);

    // make sure there is no exceptions in the queue
    if (!handle_exception(q)) {
        return -1;
    }

    for (auto p : ptrs) {
        allocator.deallocate(p.first);
        allocator.deallocate(p.second);
    }

    return 0;
}
