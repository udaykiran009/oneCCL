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

void do_regular(ccl::communicator* comm,
                ccl::coll_attr& coll_attr,
                coll_list_t& colls,
                req_list_t& reqs)
{
    char* match_id = (char*)coll_attr.match_id;

    reqs.reserve(colls.size() * BUF_COUNT);

    /* warm up */
    PRINT_BY_ROOT("do warm up");
    coll_attr.to_cache = 0;
    for (size_t count = 1; count < ELEM_COUNT; count *= 2)
    {
        for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++)
        {
            auto& coll = colls[coll_idx];
            for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
            {
                // snprintf(match_id, MATCH_ID_SIZE, "coll_%s_%zu_count_%zu_buf_%zu",
                //          coll->name(), coll_idx, count, buf_idx);
                // PRINT_BY_ROOT("start_coll: %s, count %zu, buf_idx %zu", coll->name(), count, buf_idx);
                coll->start(count, buf_idx, coll_attr, reqs);
            }
        }
        for (auto &req : reqs)
        {
            req->wait();
        }
        reqs.clear();
    }

    /* benchmark with multiple equal sized buffer per collective */
    PRINT_BY_ROOT("do multi-buffers benchmark");
    coll_attr.to_cache = 1;
    for (size_t count = 1; count <= ELEM_COUNT; count *= 2)
    {
        try
        {
            double t = 0;
            for (size_t iter_idx = 0; iter_idx < ITERS; iter_idx++)
            {
                for (auto& coll : colls)
                {
                    coll->prepare(count);
                }

                double t1 = when();
                for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++)
                {
                    auto& coll = colls[coll_idx];
                    for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
                    {
                        snprintf(match_id, MATCH_ID_SIZE, "coll_%s_%zu_count_%zu_buf_%zu",
                                 coll->name(), coll_idx, count, buf_idx);
                        coll->start(count, buf_idx, coll_attr, reqs);
                    }
                }
                for (auto &req : reqs)
                {
                    req->wait();
                }
                double t2 = when();
                t += (t2 - t1);
            }

            reqs.clear();

            for (auto& coll : colls)
            {
                coll->finalize(count);
            }
            print_timings(*comm, &t, count,
                          sizeof(DTYPE), BUF_COUNT,
                          comm->rank(), comm->size());
        }
        catch(const std::exception& ex)
        {
            ASSERT(0, "error on count %zu, reason: %s", count, ex.what());
        }
    }

    comm->barrier();

    /* benchmark with single buffer per collective */
    PRINT_BY_ROOT("do single-buffer benchmark");
    coll_attr.to_cache = 1;
    for (size_t count = BUF_COUNT; count <= SINGLE_ELEM_COUNT; count *= 2)
    {
        try
        {
            double t = 0;
            for (size_t iter_idx = 0; iter_idx < ITERS; iter_idx++)
            {
                double t1 = when();
                for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++)
                {
                    auto& coll = colls[coll_idx];
                    snprintf(match_id, MATCH_ID_SIZE, "coll_%s_%zu_single_count_%zu",
                             coll->name(), coll_idx, count);
                    coll->start_single(count, coll_attr, reqs);
                }
                for (auto &req : reqs)
                {
                    req->wait();
                }
                double t2 = when();
                t += (t2 - t1);

                reqs.clear();
            }
            print_timings(*comm, &t, count,
                          sizeof(DTYPE), 1,
                          comm->rank(), comm->size());
        } catch (...)
        {
            ASSERT(0, "error on count %zu", count);
        }
    }

    PRINT_BY_ROOT("PASSED\n");
}

void do_unordered(ccl::communicator* comm,
                  ccl::coll_attr& coll_attr,
                  coll_list_t& colls,
                  req_list_t& reqs)
{
    std::set<std::string> match_ids;
    char* match_id = (char*)coll_attr.match_id;
    size_t rank = comm->rank();

    reqs.reserve(colls.size() * BUF_COUNT * (log2(ELEM_COUNT) + 1));

    PRINT_BY_ROOT("do unordered test");
    coll_attr.to_cache = 1;

    for (size_t count = 1; count <= ELEM_COUNT; count *= 2)
    {
        try
        {
            if (rank % 2)
            {
                for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++)
                {
                    auto& coll = colls[coll_idx];
                    for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
                    {
                        snprintf(match_id, MATCH_ID_SIZE, "coll_%s_%zu_count_%zu_buf_%zu",
                                 coll->name(), coll_idx, count, buf_idx);
                        coll->start(count, buf_idx, coll_attr, reqs);
                        match_ids.emplace(match_id);
                    }
                }
            }
            else
            {
                for (size_t coll_idx = 0; coll_idx < colls.size(); coll_idx++)
                {
                    size_t real_coll_idx = colls.size() - coll_idx - 1;
                    auto& coll = colls[real_coll_idx];
                    for (size_t buf_idx = 0; buf_idx < BUF_COUNT; buf_idx++)
                    {
                        size_t real_buf_idx = BUF_COUNT - buf_idx - 1;
                        snprintf(match_id, MATCH_ID_SIZE, "coll_%s_%zu_count_%zu_buf_%zu",
                                 coll->name(), real_coll_idx, count, real_buf_idx);
                        coll->start(count, real_buf_idx, coll_attr, reqs);
                        match_ids.insert(std::string(match_id));
                    }
                }
            }
        }
        catch (...)
        {
            ASSERT(0, "error on count %zu", count);
        }
    }

    ASSERT(match_ids.size() == reqs.size(),
           "unexpected match_ids.size %zu, expected %zu",
           match_ids.size(), reqs.size());

    try
    {
        for (auto &req : reqs)
        {
            req->wait();
        }
    }
    catch (...)
    {
        ASSERT(0, "error on coll completion");
    }

    PRINT_BY_ROOT("PASSED\n");
}

template<class Dtype>
void create_cpu_colls(std::list<std::string>& names, coll_list_t& colls)
{
    using namespace sparse_detail;
    using incremental_index_int_sparse_strategy = 
                                sparse_allreduce_strategy_impl<int, 
                                sparse_detail::incremental_indices_distributor>;
    using incremental_index_bfp16_sparse_strategy = 
                                sparse_allreduce_strategy_impl<ccl::bfp16, 
                                sparse_detail::incremental_indices_distributor>;

    std::stringstream error_messages_stream;
    base_coll::comm = ccl::environment::instance().create_communicator();
    base_coll::stream = ccl::environment::instance().create_stream(ccl::stream_type::host, nullptr);
    for (auto names_it = names.begin(); names_it != names.end(); )
    {
        const std::string& name = *names_it;
        if (name == allgatherv_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_allgatherv_coll<Dtype>());
        }
        else if (name == allreduce_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_allreduce_coll<Dtype>());
        }
        else if (name == bcast_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_bcast_coll<Dtype>());
        }
        else if (name == reduce_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_reduce_coll<Dtype>());
        }
        else if (name == alltoall_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_alltoall_coll<Dtype>);
        }
        else if (name == alltoallv_strategy_impl::class_name())
        {
            colls.emplace_back(new cpu_alltoallv_coll<Dtype>);
        }
        else if (name.find(incremental_index_int_sparse_strategy::class_name()) != std::string::npos)
        {
            if (name.find(incremental_index_bfp16_sparse_strategy::class_name()) != std::string::npos)
            {
                if (is_bfp16_enabled() == 0)
                {
                    error_messages_stream << "BFP16 is not supported for current CPU, skipping " << name << ".\n";
                    names_it = names.erase(names_it);
                    continue;
                }
#ifdef CCL_BFP16_COMPILER
                const std::string args = name.substr(name.find(incremental_index_bfp16_sparse_strategy::class_name()) +
                                                     std::strlen(incremental_index_bfp16_sparse_strategy::class_name()));
                colls.emplace_back(new cpu_sparse_allreduce_coll<ccl::bfp16, int64_t,
                                                                 sparse_detail::incremental_indices_distributor>(args,
                                                                                  sizeof(float) / sizeof(ccl::bfp16),
                                                                                  sizeof(float) / sizeof(ccl::bfp16)));
#else
                error_messages_stream << "BFP16 is not supported by current compiler, skipping " << name << ".\n";
                names_it = names.erase(names_it);
                continue;
#endif
            }
            else
            {
                const std::string args = name.substr(name.find(incremental_index_int_sparse_strategy::class_name()) +
                                                     std::strlen(incremental_index_int_sparse_strategy::class_name()));
                colls.emplace_back(new cpu_sparse_allreduce_coll<Dtype, int>(args));
            }
        }
        else
        {
            ASSERT(0, "create_colls error, unknown coll name: %s", name.c_str());
        }
        ++names_it;
    }

    const std::string& coll_processing_log = error_messages_stream.str();
    if (!coll_processing_log.empty())
    {
        std::cerr << "WARNING:\n" << coll_processing_log << std::endl;
    }
    
    if (colls.empty())
    {
        throw std::logic_error(std::string(__FUNCTION__) + 
                               " - empty colls, reason: " + coll_processing_log);
    }
}

#ifdef CCL_ENABLE_SYCL
template<class Dtype>
void create_sycl_colls(std::list<std::string>& names, coll_list_t& colls)
{

    using incremental_index_int_sparse_strategy = 
                            sparse_allreduce_strategy_impl<int, 
                                sparse_detail::incremental_indices_distributor>;
    using incremental_index_bfp16_sparse_strategy = 
                            sparse_allreduce_strategy_impl<ccl::bfp16, 
                                sparse_detail::incremental_indices_distributor>;
            
    std::stringstream error_messages_stream;
    base_coll::comm = ccl::environment::instance().create_communicator();
    base_coll::stream = ccl::environment::instance().create_stream(ccl::stream_type::device, &sycl_queue);
    for (auto names_it = names.begin(); names_it != names.end(); )
    {
        const std::string& name = *names_it;
        
        if (name == allgatherv_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_allgatherv_coll<Dtype>());
        }
        else if (name == allreduce_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_allreduce_coll<Dtype>());
        }
        else if (name == alltoall_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_alltoall_coll<Dtype>());
        }
        else if (name == alltoallv_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_alltoallv_coll<Dtype>());
        }
        else if (name == bcast_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_bcast_coll<Dtype>());
        }
        else if (name == reduce_strategy_impl::class_name())
        {
            colls.emplace_back(new sycl_reduce_coll<Dtype>());
        }
        else if (name.find(incremental_index_int_sparse_strategy::class_name()) != std::string::npos)
        {
            // TODO case is not supported yet
            if (true)
            {
                error_messages_stream << "SYCL coll: skipping " << name << ", because it is not supported yet.\n";
                names_it = names.erase(names_it);
                continue;
            }
            
            const std::string args = name.substr(name.find(incremental_index_int_sparse_strategy::class_name()) +
                                                 std::strlen(incremental_index_int_sparse_strategy::class_name()));
            colls.emplace_back(new sycl_sparse_allreduce_coll<Dtype, int>(args));
        }
        else if (name.find(incremental_index_bfp16_sparse_strategy::class_name()) != std::string::npos)
        {
            // TODO case is not supported yet
            if (true)
            {
                error_messages_stream << "SYCL coll: skipping " << name << ", because it is not supported yet.\n";
                names_it = names.erase(names_it);
                continue;
            }
            
            if (is_bfp16_enabled() == 0)
            {
                error_messages_stream << "SYCL BFP16 is not supported for current CPU, skipping " << name << ".\n";
                names_it = names.erase(names_it);
                continue;
            }
#ifdef CCL_BFP16_COMPILER
            const std::string args = name.substr(name.find(incremental_index_bfp16_sparse_strategy::class_name()) +
                                                 std::strlen(incremental_index_bfp16_sparse_strategy::class_name()));
            colls.emplace_back(new sycl_sparse_allreduce_coll<ccl::bfp16, int64_t,
                                                              sparse_detail::incremental_indices_distributor>(args,
                                                                               sizeof(float) / sizeof(ccl::bfp16),
                                                                               sizeof(float) / sizeof(ccl::bfp16)));
#else
            error_messages_stream << "SYCL BFP16 is not supported by current compiler, skipping " << name << ".\n";
            names_it = names.erase(names_it);
            continue;
#endif
        }
        else
        {
            ASSERT(0, "create_colls error, unknown coll name: %s", name.c_str());
        }
        
        ++names_it;
    }
    
    const std::string& coll_processing_log = error_messages_stream.str();
    if (!coll_processing_log.empty())
    {
        std::cerr << "WARNING: " << coll_processing_log << std::endl;
    }
    
    if (colls.empty())
    {
        throw std::logic_error(std::string(__FUNCTION__) + 
                               " - empty colls, reason: " + coll_processing_log);
    }
}
#endif /* CCL_ENABLE_SYCL */

template<class Dtype>
void create_colls(std::list<std::string>& names, ccl::stream_type backend_type, coll_list_t& colls)
{
    switch (backend_type)
    {
        case ccl::stream_type::host:
            create_cpu_colls<Dtype>(names, colls);
            break;
        case ccl::stream_type::device:
#ifdef CCL_ENABLE_SYCL
            create_sycl_colls<Dtype>(names, colls);
#else
            ASSERT(0, "SYCL backend is requested but CCL_ENABLE_SYCL is not defined");
#endif
            break;
        default:
            ASSERT(0, "unknown backend %d", (int)backend_type);
            break;
    }

}

int main(int argc, char *argv[])
{
    if (argc > 4)
    {
        PRINT("%s", help_message);
        return -1;
    }

    std::string backend_str = (argc > 1) ? std::string(argv[1]) : DEFAULT_BACKEND;
    std::set<std::string> suppored_backends { "cpu" };
#ifdef CCL_ENABLE_SYCL
    suppored_backends.insert("sycl");
#endif

    std::stringstream sstream;
    if (suppored_backends.find(backend_str) == suppored_backends.end())
    {
        PRINT("unsupported backend: %s", backend_str.c_str());

        std::copy(suppored_backends.begin(), suppored_backends.end(),
                  std::ostream_iterator<std::string>(sstream, " "));
        PRINT("supported backends: %s", sstream.str().c_str());
        PRINT("%s", help_message);
        return -1;
    }

    ccl::stream_type backend_type = ccl::stream_type::host;
    if (backend_str == "sycl")
        backend_type = ccl::stream_type::device;

    std::string loop_str = (argc > 2) ? std::string(argv[2]) : DEFAULT_LOOP;
    std::set<std::string> suppored_loops { "regular", "unordered" };
    if (suppored_loops.find(loop_str) == suppored_loops.end())
    {
        PRINT("unsupported loop: %s", loop_str.c_str());

        std::copy(suppored_loops.begin(), suppored_loops.end(),
                  std::ostream_iterator<std::string>(sstream, " "));
        PRINT("supported loops: %s", sstream.str().c_str());
        PRINT("%s", help_message);
        return -1;
    }

    loop_type_t loop = LOOP_REGULAR;
    if (loop_str == "unordered")
    {
        loop = LOOP_UNORDERED;
        setenv("CCL_UNORDERED_COLL", "1", 1);
    }

    ccl::coll_attr coll_attr{};

    std::list<std::string> coll_names;
    coll_list_t colls;
    req_list_t reqs;

    char match_id[MATCH_ID_SIZE] {'\0'};
    coll_attr.match_id = match_id;


    try
    {
        coll_names = tokenize((argc == 4) ? argv[3] : DEFAULT_COLL_LIST, ',');
        create_colls<DTYPE>(coll_names, backend_type, colls);
    }
    catch (const std::runtime_error& e)
    {
        ASSERT(0, "cannot create coll objects: %s\n%s", e.what(), help_message);
    }
    catch (const std::logic_error& e)
    {
        std::cerr << "Cannot launch benchmark: " << e.what() << std::endl;
        return -1;
    }

    ccl::communicator* comm = base_coll::comm.get();
    if (colls.empty())
    {
        PRINT_BY_ROOT("%s", help_message);
        ASSERT(0, "unexpected coll list");
    }

    int check_values = 1;

    comm->barrier();

    std::copy(coll_names.begin(), coll_names.end(),
              std::ostream_iterator<std::string>(sstream, " "));

    PRINT_BY_ROOT("start colls: %s, iters: %d, buf_count: %d, ranks %zu, check_values %d, backend %s, loop %s",
                  sstream.str().c_str(), ITERS, BUF_COUNT, comm->size(), check_values,
                  backend_str.c_str(), loop_str.c_str());

    for (auto& coll : colls)
    {
        coll->check_values = check_values;
    }

    switch (loop)
    {
        case LOOP_REGULAR:
            do_regular(comm, coll_attr, colls, reqs);
            break;
        case LOOP_UNORDERED:
            do_unordered(comm, coll_attr, colls, reqs);
            break;
        default:
            ASSERT(0, "unknown loop %d", loop);
            break;
    }

    base_coll::comm.reset();
    base_coll::stream.reset();
    return 0;
}
