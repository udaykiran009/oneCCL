#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <iostream>
#include <iterator>
#include <list>
#include <memory>
#include <set>
#include <sstream>
#include <tuple>
#include <unordered_map>

#include "benchmark.hpp"
#include "declarations.hpp"
#include "transport_impl.hpp"

void do_regular(ccl::communicator& service_comm,
                bench_exec_attr& bench_attr,
                coll_list_t& all_colls,
                req_list_t& reqs,
                const user_options_t& options) {

    std::stringstream match_id_stream;

    for (auto dtype : all_dtypes) {
        coll_list_t colls;
        std::string dtype_name;

        std::copy_if(all_colls.begin(),
                     all_colls.end(),
                     std::back_inserter(colls),
                     [dtype](const typename coll_list_t::value_type coll) {
                         return dtype == coll->get_dtype();
                     });
        if (colls.empty())
            continue;

        dtype_name = find_str_val(dtype_names, dtype);

        for (const auto& reduction : options.reductions) {
            ccl::reduction reduction_op;

            if (!find_key_val(reduction_op, reduction_names, reduction))
                continue;

            PRINT_BY_ROOT(
                service_comm, "\ndtype: %s\nreduction: %s\n", dtype_name.c_str(), reduction.c_str());

            reqs.reserve(colls.size() * options.buf_count);

            /* warm up */
            PRINT_BY_ROOT(service_comm, "do warm up");

            bench_attr.reduction = reduction_op;
            bench_attr.set<ccl::operation_attr_id::to_cache>(true);

            ccl::barrier(service_comm);

            for (size_t count = options.min_elem_count; count <= options.max_elem_count;
                 count *= 2) {

                for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++) {

                    auto& coll = colls[coll_idx];

                    size_t iter_count =
                        get_iter_count(count * ccl::get_datatype_size(coll->get_dtype()), options.warmup_iters);

                    for (size_t iter_idx = 0; iter_idx < iter_count; iter_idx++) {

                        for (size_t buf_idx = 0; buf_idx < options.buf_count; buf_idx++) {
                            match_id_stream << "coll_" << coll->name()
                                            << "_" << coll_idx << "_count_" << count 
                                            << "_buf_" << buf_idx;
                            bench_attr.set<ccl::operation_attr_id::match_id>(ccl::string_class(match_id_stream.str()));
                            match_id_stream.str("");
                            coll->start(count, buf_idx, bench_attr, reqs);
                        }

                        for (auto& req : reqs) {
                            req.wait();
                        }
                        reqs.clear();
                    }
                }
            }

            std::ostringstream scolls;
            std::copy(options.coll_names.begin(),
                      options.coll_names.end(),
                      std::ostream_iterator<std::string>{ scolls, " " });

            ccl::barrier(service_comm);

            /* benchmark with multiple equal sized buffer per collective */
            if (options.buf_type == BUF_MULTI) {
                PRINT_BY_ROOT(service_comm,
                              "do multi-buffers benchmark\n"
                              "#------------------------------------------------------------\n"
                              "# Benchmarking: %s\n"
                              "# processes: %zu\n"
                              "#------------------------------------------------------------\n"
                              "%10s %13s %18s %11s",
                              scolls.str().c_str(),
                              service_comm.size(),
                              "#bytes",
                              "avg[usec]",
                              "avg_per_buf[usec]",
                              "stddev[%]");
                bench_attr.set<ccl::operation_attr_id::to_cache>(true);
                for (size_t count = options.min_elem_count; count <= options.max_elem_count;
                     count *= 2) {
                    try {
                        // we store times for each collective separately,
                        // but aggregate over buffers and iterations
                        std::vector<double> coll_timers(colls.size(), 0);
                        for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++) {

                            auto& coll = colls[coll_idx];

                            ccl::barrier(service_comm);

                            double t1 = 0, t2 = 0, t = 0;

                            size_t iter_count =
                                get_iter_count(count * ccl::get_datatype_size(coll->get_dtype()), options.iters);

                            for (size_t iter_idx = 0; iter_idx < iter_count; iter_idx++) {

                                // collective is configured to handle only
                                // options.buf_count many buffers/executions 'at once'.
                                // -> check cannot combine executions over iterations
                                // -> wait and check and must be in this loop nest
                                if (options.check_values) {
                                    coll->prepare(count);
                                }

                                ccl::barrier(service_comm);

                                t1 = when();

                                for (size_t buf_idx = 0; buf_idx < options.buf_count; buf_idx++) {
                                    match_id_stream << "coll_" << coll->name()
                                                    << "_" << coll_idx << "_count_" << count 
                                                    << "_buf_" << buf_idx;
                                    bench_attr.set<ccl::operation_attr_id::match_id>(ccl::string_class(match_id_stream.str()));
                                    match_id_stream.str("");
                                    coll->start(count, buf_idx, bench_attr, reqs);
                                }

                                for (auto& req : reqs) {
                                    req.wait();
                                }
                                reqs.clear();

                                t2 = when();
                                t += (t2 - t1);

                                if (options.check_values) {
                                    coll->finalize(count);
                                }
                            }
                            coll_timers[coll_idx] += t;
                        }
                        print_timings(service_comm, coll_timers, options, count, dtype, reduction_op);
                    }
                    catch (const std::exception& ex) {
                        ASSERT(0, "error on count %zu, reason: %s", count, ex.what());
                    }
                }
            }
            else {
                /* benchmark with single buffer per collective */
                PRINT_BY_ROOT(service_comm,
                              "do single-buffer benchmark\n"
                              "#--------------------------------------\n"
                              "# Benchmarking: %s\n"
                              "# processes: %zu\n"
                              "#--------------------------------------\n"
                              "%10s %12s %11s",
                              scolls.str().c_str(),
                              service_comm.size(),
                              "#bytes",
                              "avg[usec]",
                              "stddev[%]");
                size_t min_elem_count = options.min_elem_count * options.buf_count;
                size_t max_elem_count = options.max_elem_count * options.buf_count;

                bench_attr.set<ccl::operation_attr_id::to_cache>(true);
                for (size_t count = min_elem_count; count <= max_elem_count; count *= 2) {
                    try {
                        // we store times for each collective separately,
                        // but aggregate over iterations
                        std::vector<double> coll_timers(colls.size(), 0);

                        double t1 = 0, t2 = 0;

                        for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++) {

                            auto& coll = colls[coll_idx];

                            size_t iter_count =
                                get_iter_count(count * ccl::get_datatype_size(coll->get_dtype()), options.iters);

                            ccl::barrier(service_comm);

                            t1 = when();

                            for (size_t iter_idx = 0; iter_idx < iter_count; iter_idx++) {
                                match_id_stream << "coll_" << coll->name()
                                                << "_" << coll_idx << "_single_count_" << count;
                                bench_attr.set<ccl::operation_attr_id::match_id>(ccl::string_class(match_id_stream.str()));
                                match_id_stream.str("");

                                coll->start_single(count, bench_attr, reqs);

                                for (auto& req : reqs) {
                                    req.wait();
                                }
                                reqs.clear();
                            }

                            t2 = when();

                            coll_timers[coll_idx] += (t2 - t1);
                        }

                        print_timings(service_comm, coll_timers, options, count, dtype, reduction_op);
                    }
                    catch (...) {
                        ASSERT(0, "error on count %zu", count);
                    }
                }
                PRINT_BY_ROOT(service_comm, "PASSED\n");
            }
        }
    }
}

void do_unordered(ccl::communicator& service_comm,
                  bench_exec_attr& bench_attr,
                  coll_list_t& all_colls,
                  req_list_t& reqs,
                  const user_options_t& options) {

    std::set<ccl::string_class> match_ids;
    std::stringstream match_id_stream;

    for (auto dtype : all_dtypes) {
        coll_list_t colls;
        std::string dtype_name;

        std::copy_if(all_colls.begin(),
                     all_colls.end(),
                     std::back_inserter(colls),
                     [dtype](const typename coll_list_t::value_type coll) {
                         return dtype == coll->get_dtype();
                     });

        if (colls.empty())
            continue;

        dtype_name = find_str_val(dtype_names, dtype);
        for (const auto& reduction : options.reductions) {
            ccl::reduction reduction_op;

            if (!find_key_val(reduction_op, reduction_names, reduction))
                continue;

            PRINT_BY_ROOT(
                service_comm, "\ndtype: %s\nreduction: %s\n", dtype_name.c_str(), reduction.c_str());

            size_t rank = service_comm.rank();

            reqs.reserve(colls.size() * options.buf_count * (log2(options.max_elem_count) + 1));

            PRINT_BY_ROOT(service_comm, "do unordered test");
            bench_attr.reduction = reduction_op;
            bench_attr.set<ccl::operation_attr_id::to_cache>(true);

            for (size_t count = options.min_elem_count; count <= options.max_elem_count;
                 count *= 2) {
                try {
                    if (rank % 2) {
                        for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++) {
                            auto& coll = colls[coll_idx];
                            for (size_t buf_idx = 0; buf_idx < options.buf_count; buf_idx++) {
                                match_id_stream << "coll_" << coll->name()
                                                << "_" << coll_idx << "_count_" << count 
                                                << "_buf_" << buf_idx;
                                bench_attr.set<ccl::operation_attr_id::match_id>(ccl::string_class(match_id_stream.str()));
                                match_ids.insert(match_id_stream.str());
                                match_id_stream.str("");
                                coll->start(count, buf_idx, bench_attr, reqs);
                                
                            }
                        }
                    }
                    else {
                        for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++) {
                            size_t real_coll_idx = colls.size() - coll_idx - 1;
                            auto& coll = colls[real_coll_idx];
                            for (size_t buf_idx = 0; buf_idx < options.buf_count; buf_idx++) {
                                size_t real_buf_idx = options.buf_count - buf_idx - 1;
                                match_id_stream << "coll_" << coll->name()
                                                << "_" << real_coll_idx << "_count_" << count 
                                                << "_buf_" << real_buf_idx;
                                bench_attr.set<ccl::operation_attr_id::match_id>(ccl::string_class(match_id_stream.str()));
                                match_ids.insert(match_id_stream.str());
                                match_id_stream.str("");
                                coll->start(count, real_buf_idx, bench_attr, reqs);
                            }
                        }
                    }
                }
                catch (...) {
                    ASSERT(0, "error on count %zu", count);
                }
            }

            ASSERT(match_ids.size() == reqs.size(),
                   "unexpected match_ids.size %zu, expected %zu",
                   match_ids.size(),
                   reqs.size());

            try {
                for (auto& req : reqs) {
                    req.wait();
                }
            }
            catch (...) {
                ASSERT(0, "error on coll completion");
            }
            PRINT_BY_ROOT(service_comm, "PASSED\n");
        }
    }
}

template <class Dtype>
void create_cpu_colls(bench_init_attr& init_attr,
                      user_options_t& options,
                      coll_list_t& colls) {
    using namespace sparse_detail;
    using incremental_index_int_sparse_strategy =
        sparse_allreduce_strategy_impl<int, sparse_detail::incremental_indices_distributor>;
    using incremental_index_bf16_sparse_strategy =
        sparse_allreduce_strategy_impl<ccl::bf16, sparse_detail::incremental_indices_distributor>;

    std::stringstream error_messages_stream;

    for (auto names_it = options.coll_names.begin(); names_it != options.coll_names.end();) {
        const std::string& name = *names_it;
        if (name == allgatherv_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_allgatherv_coll<Dtype>(init_attr));
        }
        else if (name == allreduce_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_allreduce_coll<Dtype>(init_attr));
        }
        else if (name == alltoall_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_alltoall_coll<Dtype>(init_attr));
        }
        else if (name == alltoallv_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_alltoallv_coll<Dtype>(init_attr));
        }
        else if (name == bcast_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_bcast_coll<Dtype>(init_attr));
        }
        else if (name == reduce_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_reduce_coll<Dtype>(init_attr));
        }
        else if (name == reduce_scatter_strategy_impl::class_name()) {
            colls.emplace_back(new cpu_reduce_scatter_coll<Dtype>(init_attr));
        }
        else if (name.find(incremental_index_int_sparse_strategy::class_name()) !=
                 std::string::npos) {
            if (name.find(incremental_index_bf16_sparse_strategy::class_name()) !=
                std::string::npos) {
                if (is_bf16_enabled() == 0) {
                    error_messages_stream << "bfloat16 is not supported for current CPU, skipping "
                                          << name << ".\n";
                    names_it = options.coll_names.erase(names_it);
                    continue;
                }
#ifdef CCL_bf16_COMPILER
                colls.emplace_back(
                    new cpu_sparse_allreduce_coll<ccl::bf16,
                                                  int64_t,
                                                  sparse_detail::incremental_indices_distributor>(
                        init_attr,
                        sizeof(float) / sizeof(ccl::bf16),
                        sizeof(float) / sizeof(ccl::bf16)));
#else
                error_messages_stream << "bfloat16 is not supported by current compiler, skipping "
                                      << name << ".\n";
                names_it = options.coll_names.erase(names_it);
                continue;
#endif
            }
            else {
                colls.emplace_back(new cpu_sparse_allreduce_coll<Dtype, int64_t>(init_attr));
            }
        }
        else {
            ASSERT(0, "create_colls error, unknown coll name: %s", name.c_str());
        }
        ++names_it;
    }

    const std::string& coll_processing_log = error_messages_stream.str();
    if (!coll_processing_log.empty()) {
        std::cerr << "WARNING:\n" << coll_processing_log << std::endl;
    }

    if (colls.empty()) {
        throw std::logic_error(std::string(__FUNCTION__) +
                               " - empty colls, reason: " + coll_processing_log);
    }
}

#ifdef CCL_ENABLE_SYCL
template <class Dtype>
void create_sycl_colls(bench_init_attr& init_attr,
                       user_options_t& options,
                       coll_list_t& colls) {
    using incremental_index_int_sparse_strategy =
        sparse_allreduce_strategy_impl<int, sparse_detail::incremental_indices_distributor>;
    using incremental_index_bf16_sparse_strategy =
        sparse_allreduce_strategy_impl<ccl::bf16, sparse_detail::incremental_indices_distributor>;

    std::stringstream error_messages_stream;

    for (auto names_it = options.coll_names.begin(); names_it != options.coll_names.end();) {
        const std::string& name = *names_it;

        if (name == allgatherv_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_allgatherv_coll<Dtype>(init_attr));
        }
        else if (name == allreduce_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_allreduce_coll<Dtype>(init_attr));
        }
        else if (name == alltoall_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_alltoall_coll<Dtype>(init_attr));
        }
        else if (name == alltoallv_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_alltoallv_coll<Dtype>(init_attr));
        }
        else if (name == bcast_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_bcast_coll<Dtype>(init_attr));
        }
        else if (name == reduce_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_reduce_coll<Dtype>(init_attr));
        }
        else if (name == reduce_scatter_strategy_impl::class_name()) {
            colls.emplace_back(new sycl_reduce_scatter_coll<Dtype>(init_attr));
        }
        else if (name.find(incremental_index_int_sparse_strategy::class_name()) !=
                 std::string::npos) {
            // TODO case is not supported yet
            if (true) {
                error_messages_stream << "SYCL coll: skipping " << name
                                      << ", because it is not supported yet.\n";
                names_it = options.coll_names.erase(names_it);
                continue;
            }
            colls.emplace_back(new sycl_sparse_allreduce_coll<Dtype, int>(init_attr));
        }
        else if (name.find(incremental_index_bf16_sparse_strategy::class_name()) !=
                 std::string::npos) {
            // TODO case is not supported yet
            if (true) {
                error_messages_stream << "SYCL coll: skipping " << name
                                      << ", because it is not supported yet.\n";
                names_it = options.coll_names.erase(names_it);
                continue;
            }

            if (is_bf16_enabled() == 0) {
                error_messages_stream << "SYCL bf16 is not supported for current CPU, skipping "
                                      << name << ".\n";
                names_it = options.coll_names.erase(names_it);
                continue;
            }
#ifdef CCL_bf16_COMPILER
            colls.emplace_back(
                new sycl_sparse_allreduce_coll<ccl::bf16,
                                               int64_t,
                                               sparse_detail::incremental_indices_distributor>(
                    init_attr,
                    sizeof(float) / sizeof(ccl::bf16),
                    sizeof(float) / sizeof(ccl::bf16)));
#else
            error_messages_stream << "SYCL bf16 is not supported by current compiler, skipping "
                                  << name << ".\n";
            names_it = options.coll_names.erase(names_it);
            continue;
#endif
        }
        else {
            ASSERT(0, "create_colls error, unknown coll name: %s", name.c_str());
        }

        ++names_it;
    }

    const std::string& coll_processing_log = error_messages_stream.str();
    if (!coll_processing_log.empty()) {
        std::cerr << "WARNING: " << coll_processing_log << std::endl;
    }

    if (colls.empty()) {
        throw std::logic_error(std::string(__FUNCTION__) +
                               " - empty colls, reason: " + coll_processing_log);
    }
}
#endif /* CCL_ENABLE_SYCL */

template <class Dtype>
void create_colls(bench_init_attr& init_attr, user_options_t& options, coll_list_t& colls) {
    switch (options.backend) {
        case BACKEND_HOST:
            create_cpu_colls<Dtype>(init_attr, options, colls);
            break;
        case BACKEND_SYCL:
#ifdef CCL_ENABLE_SYCL
            create_sycl_colls<Dtype>(init_attr, options, colls);
#else
            ASSERT(0, "SYCL backend is requested but CCL_ENABLE_SYCL is not defined");
#endif
            break;
        default: ASSERT(0, "unknown backend %d", (int)options.backend); break;
    }
}

/* Reason to leave a functor here: In order to call a function (create_colls())
 * with all dtypes (from ccl::datatype) the functor requires the implementation
 * of that function. */
class create_colls_func {
private:
    bench_init_attr& init_attr;
    user_options_t& options;
    coll_list_t& colls;

public:
    create_colls_func(bench_init_attr& init_attr,
                      user_options_t& options,
                      coll_list_t& colls)

            : init_attr(init_attr),
              options(options),
              colls(colls) {}

    template <class Dtype>
    void operator()(const Dtype& value) {
        if (true == std::get<0>(value)) {
            create_colls<typename Dtype::second_type>(init_attr, options, colls);
        }
    }
};

int main(int argc, char* argv[]) {
    user_options_t options;
    coll_list_t colls;
    req_list_t reqs;

    bench_init_attr init_attr;

    if (parse_user_options(argc, argv, options)) {
        print_help_usage(argv[0]);
        return -1;
    }

    auto& transport = transport_data::instance();
    transport.init_comms(options);

    ccl::communicator& service_comm = transport.get_service_comm();

    init_attr.buf_count = options.buf_count;
    init_attr.max_elem_count = options.max_elem_count;
    init_attr.ranks_per_proc = options.ranks_per_proc;
    init_attr.sycl_mem_type = options.sycl_mem_type;
    init_attr.sycl_usm_type = options.sycl_usm_type;
    init_attr.v2i_ratio = options.v2i_ratio;

    try {
        ccl_tuple_for_each(launch_dtypes, set_dtypes_func(options.dtypes));
        ccl_tuple_for_each(launch_dtypes, create_colls_func(init_attr, options, colls));
    }
    catch (const std::runtime_error& e) {
        ASSERT(0, "cannot create coll objects: %s\n", e.what());
    }
    catch (const std::logic_error& e) {
        std::cerr << "Cannot launch benchmark: " << e.what() << std::endl;
        return -1;
    }

    bench_exec_attr bench_attr{};
    bench_attr.init_all();

    print_user_options(options, service_comm);

    if (options.coll_names.empty()) {
        PRINT_BY_ROOT(service_comm, "empty coll list");
        print_help_usage(argv[0]);
        return -1;
    }

    ccl::barrier(service_comm);

    switch (options.loop) {
        case LOOP_REGULAR: {
            // open and truncate CSV file if csv-output is requested
            if (service_comm.rank() == 0 && !options.csv_filepath.empty()) {
                std::ofstream csvf;
                csvf.open(options.csv_filepath, std::ios::trunc);
                if (!csvf.is_open()) {
                    std::cerr << "Cannot open CSV file for writing: " << options.csv_filepath
                              << std::endl;
                    return -1;
                }
                // write header (column names)
                csvf << "#ranks,collective,reduction,type,typesize,#elements/buffer,#buffers,time"
                     << std::endl;
                csvf.close();
            }
            ccl::barrier(service_comm);
            do_regular(service_comm, bench_attr, colls, reqs, options);
            break;
        }
        case LOOP_UNORDERED: {
            // no timing is printed or exported here
            ccl::barrier(service_comm);
            do_unordered(service_comm, bench_attr, colls, reqs, options);
            break;
        }
        default: ASSERT(0, "unknown loop %d", options.loop); break;
    }

    return 0;
}
