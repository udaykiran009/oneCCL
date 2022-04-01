#include "sycl_base.hpp"

#include <numeric>
#include <sstream>

#include <getopt.h>

using namespace std;
using namespace sycl;

enum class exec_mode { single_allreduce, multi_allreduce };

struct run_args {
    exec_mode mode = exec_mode::multi_allreduce;
    size_t count = 2 * 1024 * 1024; // 8mb of floats
    size_t iter_count = 20;
    size_t kernel_count = 15;
    bool enable_cache = false;
    size_t skip_iter_count = 0;
    bool verbose_output = false;
    bool disable_allreduce = false;

    void print() {
        std::stringstream ss;
        ss << "Using parameters: \n";
        ss << "mode: ";
        if (mode == exec_mode::single_allreduce)
            ss << "0 - single allreduce\n";
        else
            ss << "1 - multi allreduce\n";

        ss << "count: " << count << "\n";
        ss << "iter_count: " << iter_count << "\n";
        ss << "kernel_count: " << kernel_count << "\n";
        ss << "cache: " << enable_cache << "\n";
        ss << "skip_iter_count: " << skip_iter_count << "\n";
        ss << "verbose: " << verbose_output << "\n";
        ss << "disable_allreduce: " << disable_allreduce << "\n";

        std::cout << ss.str() << std::endl;
    }
};

static run_args args;

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

        if (args.verbose_output)
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

        // TODO: how to handle single buffer mode?
        size_t allreduce_time = get_between_kernels_exec_time(prev, cur);

        kernel_times.push_back({ submit_kernel_time, update_kernel_time, allreduce_time });
    }
}

bool process_args(int argc, char *argv[], run_args &test_args) {
    char c;

    const char *short_ops = "b:c:i:k:ps:vdh";
    struct option ops[] = { { "buffer", required_argument, nullptr, 'b' },
                            { "count", required_argument, nullptr, 'c' },
                            { "iter_count", required_argument, nullptr, 'i' },
                            { "kernel_count", required_argument, nullptr, 'k' },
                            { "cache", no_argument, nullptr, 'p' },
                            { "skip_iter_count", required_argument, nullptr, 's' },
                            { "verbose", no_argument, nullptr, 'v' },
                            { "disable_allreduce", no_argument, nullptr, 'd' },
                            { "help", no_argument, nullptr, 'h' },
                            { nullptr, 0, nullptr, 0 } };

    while (true) {
        c = getopt_long(argc, argv, short_ops, ops, nullptr);
        if (c == -1)
            break;

        switch (c) {
            case 'b': {
                int val = atoi(optarg);
                if (val == 0) {
                    args.mode = exec_mode::single_allreduce;
                }
                else if (val == 1) {
                    args.mode = exec_mode::multi_allreduce;
                }
                else {
                    return false;
                }
                break;
            }
            case 'c': test_args.count = atoi(optarg); break;
            case 'i': test_args.iter_count = atoi(optarg); break;
            case 'k': test_args.kernel_count = atoi(optarg); break;
            case 'p': test_args.enable_cache = true; break;
            case 's': test_args.skip_iter_count = atoi(optarg); break;
            case 'v': test_args.verbose_output = true; break;
            case 'd': test_args.disable_allreduce = true; break;
            case 'h':
            default: return false;
        }
    }

    return true;
}

void print_help() {
    std::stringstream ss;
    auto convert_val = [](bool val) {
        if (val) {
            return "enabled";
        }
        else {
            return "disabled";
        }
    };

    ss << "Usage:\n";
    ss << "\t[-b,--buffer <mode>]:\n";
    ss << "\t   0 - single buffer mode\n";
    ss << "\t   1 - multi buffer mode\n";
    ss << "\t   default: " << (int)args.mode << "\n";

    ss << "\t[-c,--count <num elements>] (default: " << args.count << ") \n";
    ss << "\t[-i,--iter_count <num iterations>] (default: " << args.iter_count << ")\n";
    ss << "\t[-k,--kernel_count <num kernels per iteration>] (default: " << args.kernel_count
       << ")\n";

    ss << "\t[-p,--cache](default: " << convert_val(args.enable_cache) << ")\n";

    ss << "\t[-s,--skip_iter_count <num iters to skip wait> (default: " << args.skip_iter_count
       << ")\n";
    ss << "\t[-v,--verbose](default: " << convert_val(args.verbose_output) << ")\n";
    ss << "\t[-d,--disable_allreduce](default: " << convert_val(args.disable_allreduce) << ")\n";
    ss << "\t[-h,--help] - print help message\n";

    std::cout << ss.str() << std::endl;
}

int main(int argc, char *argv[]) {
    if (!process_args(argc, argv, args)) {
        print_help();
        return 1;
    }

    args.print();

    exec_mode mode = args.mode;
    size_t count = args.count;

    int size = 0;
    int rank = 0;

    size_t iter_count = args.iter_count;
    size_t kernel_count = args.kernel_count;

    size_t byte_count = count * 4;

    ccl::init();
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    sycl::property_list props{ sycl::property::queue::in_order{},
                               sycl::property::queue::enable_profiling{} };

    sycl::queue q;
    if (!create_sycl_queue("gpu", rank, q, props))
        return -1;

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
    // for single buffer, only 1 allocation is needed
    std::vector<std::pair<float *, float *>> ptrs(kernel_count);
    // allocate all the buffers
    if (mode == exec_mode::multi_allreduce) {
        for (size_t i = 0; i < kernel_count; i++) {
            float *weight_buf = allocator.allocate(byte_count, usm::alloc::device);
            float *weight_allreduce_buf = allocator.allocate(byte_count, usm::alloc::device);
            ptrs[i] = { weight_buf, weight_allreduce_buf };
        }
    }
    else {
        float *weight_buf = allocator.allocate(byte_count * kernel_count, usm::alloc::device);
        float *weight_allreduce_buf =
            allocator.allocate(byte_count * kernel_count, usm::alloc::device);
        // in case of single buffer set all ptrs with the same buffers for consistency
        for (size_t i = 0; i < kernel_count; ++i) {
            ptrs[i] = { weight_buf + byte_count / sizeof(float) * i,
                        weight_allreduce_buf + byte_count / sizeof(float) * i };
        }
    }

    std::vector<ccl::event> ccl_events;
    std::vector<std::tuple<sycl::event, sycl::event>> kernel_events;
    std::vector<std::tuple<size_t, size_t, size_t>> kernel_event_times;

    std::vector<size_t> allreduce_api_times(iter_count * kernel_count);

    std::vector<ccl::allreduce_attr> attrs;
    // prepare match_id's for each kernel
    for (size_t kernel_idx = 0; kernel_idx < kernel_count; ++kernel_idx) {
        attrs.emplace_back(ccl::create_operation_attr<ccl::allreduce_attr>());
        if (args.enable_cache) {
            auto &attr = attrs.back();

            ccl::string_class match_id = "allreduce";
            attr.set<ccl::operation_attr_id::to_cache>(true);
            match_id = match_id + std::to_string(kernel_idx);
            attr.set<ccl::operation_attr_id::match_id>(match_id);
        }
    }

    auto run_op_kernel = [&](int iter_idx, int kernel_idx) {
        float *weight_buf = ptrs[kernel_idx].first;

        if (iter_idx == 0) {
            return q.submit([&](auto &h) {
                h.parallel_for(count, [=](auto id) {
                    // initial weight in first iteration
                    weight_buf[id] = kernel_idx * (rank + 1);
                });
            });
        }
        else {
            return q.submit([&](auto &h) {
                h.parallel_for(count, [=](auto id) {
                    // make weight differ in each iteration
                    weight_buf[id] = weight_buf[id] + (kernel_idx * (rank + 1));
                });
            });
        }
    };

    auto run_allreduce = [&](int iter_idx, int kernel_idx) {
        if (!args.disable_allreduce) {
            float *weight_buf = ptrs[kernel_idx].first;
            float *weight_allreduce_buf = ptrs[kernel_idx].second;
            size_t buf_count = (mode == exec_mode::single_allreduce) ? count * kernel_count : count;
            size_t num = iter_idx * kernel_count + kernel_idx;

            auto coll_start = std::chrono::high_resolution_clock::now();
            auto ccl_event = ccl::allreduce(weight_buf,
                                            weight_allreduce_buf,
                                            buf_count,
                                            ccl::reduction::sum,
                                            comm,
                                            stream,
                                            attrs[kernel_idx]);

            auto coll_end = std::chrono::high_resolution_clock::now();
            // we don't really need the associated sycl event, but just
            // call get_native() to eunsure that it doesn't break anything
            (void)ccl_event.get_native();
            // we have to store the output event so it won't be destroyed
            // too early, although we don't have to do anything with it
            // as the test uses in-order queue and ccl already syncs it with
            // internal ones that run our kernels
            ccl_events.push_back(std::move(ccl_event));

            allreduce_api_times[num] =
                std::chrono::duration_cast<std::chrono::nanoseconds>(coll_end - coll_start).count();
        }
    };

    auto run_upd_kernel = [&](int iter_idx, int kernel_idx) {
        float *weight_buf = ptrs[kernel_idx].first;
        float *weight_allreduce_buf = ptrs[kernel_idx].second;

        return q.submit([&](auto &h) {
            h.parallel_for(count, [=](auto id) {
                // update weight in each iteration
                weight_buf[id] = weight_allreduce_buf[id] / size;
            });
        });
    };

    auto multi_allreduce_iteration = [&](int iter_idx) {
        for (size_t kernel_idx = 0; kernel_idx < kernel_count; kernel_idx++) {
            // step 1: FWK kernel submission
            auto submit_event = run_op_kernel(iter_idx, kernel_idx);

            // step 2: Call CCL AllReduce API
            run_allreduce(iter_idx, kernel_idx);

            // step 3: Weight update
            auto update_event = run_upd_kernel(iter_idx, kernel_idx);

            kernel_events.push_back({ submit_event, update_event });
        }
    };

    auto single_allreduce_iteration = [&](int iter_idx) {
        std::vector<sycl::event> submit_events;
        std::vector<sycl::event> update_events;
        for (size_t kernel_idx = 0; kernel_idx < kernel_count; kernel_idx++) {
            // step 1: FWK kernel submission
            submit_events.push_back(run_op_kernel(iter_idx, kernel_idx));
        }

        // step 2: Call CCL AllReduce API
        run_allreduce(iter_idx, 0);

        for (size_t kernel_idx = 0; kernel_idx < kernel_count; kernel_idx++) {
            // step 3: Weight update
            update_events.push_back(run_upd_kernel(iter_idx, kernel_idx));
        }

        for (size_t kernel_idx = 0; kernel_idx < kernel_count; ++kernel_idx) {
            kernel_events.push_back({ submit_events[kernel_idx], update_events[kernel_idx] });
        }
    };

    for (size_t iter_idx = 0; iter_idx < iter_count; ++iter_idx) {
        if (mode == exec_mode::multi_allreduce) {
            multi_allreduce_iteration(iter_idx);
        }
        else {
            single_allreduce_iteration(iter_idx);
        }

        if (args.skip_iter_count != 0 &&
            ((iter_idx % args.skip_iter_count != 0) && (iter_idx != args.iter_count - 1)))
            continue;

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
    if (!args.disable_allreduce) {
        auto weight_host_buf = allocator.allocate(byte_count, usm::alloc::host);
        q.memcpy(weight_host_buf, ptrs[kernel_count - 1].first, byte_count);
        q.wait();

        // here's how the formula is calculated for expected value:
        //
        // on 0-th iteration op_kernel fills buffer with values:
        // init_value = (kernel_idx * (rank + 1))
        //
        // after allreduce buffer contains identical values which is sum across all ranks:
        // init_sum = kernel_idx * sum(i from 1 to size) = kernel_idx * (size * (size + 1)) / 2
        //
        // after update_kernel buffer values are adjusted, this is result of 0-th iteration:
        // r0 = init_sum / size
        //
        // since 1-st iteration op_kernel sums previous value r(i-1) and init_value and does allreduce again
        // this is result of first iteration:
        // r(i) = (r(i-1) * size + init_sum) / size
        //
        // n = iter_count, on last iteration with index (n-1) result is the following:
        // r(n-1) = (r(n-2) * size + init_sum) / size
        //
        // expanding formula:
        // r(n-1) =
        //  ((r(n-3) * size + init_sum) / size * size + init_sum) / size =
        //  (r(n-3) * size + 2 * init_sum) / size =
        //  (r(n-4) * size + 3 * init_sum) / size =
        //  (r0 * size + (n-1) * init_sum) / size =
        //  (init_sum / size * size + (n-1) * init_sum) / size =
        //  n * init_sum / size
        //
        // r(n-1) = iter_count / size * kernel_idx * (size * (size + 1)) / 2
        //
        // use only last kernel for result checking, i.e. with kernel_idx = (kernel_count - 1)

        const float check_value =
            (iter_count / static_cast<float>(size)) * (kernel_count - 1) * (size * (size + 1)) / 2;

        for (size_t n = 0; n < count; n++) {
            if (fabs(weight_host_buf[n] - check_value) >= std::numeric_limits<float>::epsilon()) {
                std::cout << "FAILED\n" << std::endl;
                std::cout << "expected: " << weight_host_buf[n] << ", got: " << check_value
                          << std::endl;
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

    if (mode == exec_mode::multi_allreduce) {
        for (auto p : ptrs) {
            allocator.deallocate(p.first);
            allocator.deallocate(p.second);
        }
    }
    else {
        allocator.deallocate(ptrs[0].first);
        allocator.deallocate(ptrs[0].second);
    }

    return 0;
}
