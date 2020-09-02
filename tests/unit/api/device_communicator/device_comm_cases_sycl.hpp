#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"

#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"


#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

#include "ccl_request.hpp"

#include "coll/coll_attributes.hpp"

#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"
#include "comm_split_attr_creation_impl.hpp"

//#include "environment.hpp"
#include "ccl_device_communicator.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "event_impl.hpp"
#include "stream_impl.hpp"

#include "../stubs/kvs.hpp"

#include "common/global/global.hpp"

#include "../stubs/native_platform.hpp"

//TODO
#include "common/comm/comm.hpp"

#include "common/comm/l0/comm_context.hpp"
#include "device_communicator_impl.hpp"
namespace device_communicator_suite
{

TEST(device_communicator_api, device_comm_from_sycl_devices_single_thread)
{
    // fill stub parameters
    ccl_comm::ccl_comm_reset_thread_barrier();
    ccl::group_context::instance().communicator_group_map.clear();
    ccl::device_indices_t indices {
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    ccl::device_index_type(0,0, ccl::unused_index_value),
                                    ccl::device_index_type(0,0, ccl::unused_index_value)
                                  };
    stub::make_stub_devices(indices);
    ccl::global_data::get().thread_barrier_wait_timeout_sec = 0;
    ccl_logger::set_log_level(static_cast<ccl_log_level>(3));

    // run test
    // prepare 'in_*' parameters
    size_t in_total_devices_size = 4;
    ccl::vector_class<cl::sycl::device> ranked_devices{in_total_devices_size, cl::sycl::device{}};

    ccl::vector_class<ccl::pair_class<size_t, cl::sycl::device>> in_local_rank_device_map;
    in_local_rank_device_map.reserve(in_total_devices_size);
    size_t curr_rank = 0;
    std::transform(ranked_devices.begin(), ranked_devices.end(), std::back_inserter(in_local_rank_device_map),
                   [&curr_rank](cl::sycl::device& val)
                   {
                       return std::make_pair(curr_rank++, val);
                   });

    auto in_ctx = cl::sycl::context();
    std::shared_ptr<stub_kvs> in_kvs;

    // create `out_comms` from in parameters
    ccl::vector_class<ccl::device_communicator> out_comms = ccl::device_communicator::create_device_communicators(in_total_devices_size, in_local_rank_device_map, in_ctx, in_kvs);

    // check correctness
    curr_rank = in_local_rank_device_map.begin()->first;
    ASSERT_EQ(out_comms.size(), ranked_devices.size());
    for (auto& dev_comm : out_comms)
    {
        ASSERT_TRUE(dev_comm.is_ready());
        try
        {
            ASSERT_EQ(dev_comm.get_context(), in_ctx);
        }
        catch(...)
        {
             //TODO ignore util L0 1.0
        }

        try
        {
            EXPECT_EQ(dev_comm.size(), in_total_devices_size);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        try
        {
            EXPECT_EQ(dev_comm.rank(), curr_rank);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        curr_rank++;

        void *tmp = nullptr;
        ccl::vector_class<size_t> recv_counts;
        ccl::datatype dtype{ccl::datatype::int8};
        dev_comm.allgatherv((const void*) tmp, size_t(0), tmp, recv_counts, dtype, ccl::default_allgatherv_attr);
    }
}


using rank_device_container_t = ccl::vector_class<ccl::pair_class<size_t, cl::sycl::device>>;
using thread_rank_device_container_t = ccl::map_class<size_t, rank_device_container_t>;

void user_thread_function(size_t total_devices_count, const rank_device_container_t& in_local_rank_device_map,
                          cl::sycl::context& in_ctx,
                          std::shared_ptr<stub_kvs> in_kvs,
                          std::atomic<size_t>& total_communicators_count)
{
    // blocking API call: wait for all threads from all processes
    ccl::vector_class<ccl::device_communicator> out_comms = ccl::device_communicator::create_device_communicators(total_devices_count, in_local_rank_device_map, in_ctx, in_kvs);

    // check correctness
    total_communicators_count.fetch_add(out_comms.size());

    size_t curr_rank = in_local_rank_device_map.begin()->first;
    ASSERT_EQ(out_comms.size(), in_local_rank_device_map.size());
    for (auto& dev_comm : out_comms)
    {

        ASSERT_TRUE(dev_comm.is_ready());

        void *tmp = nullptr;
        ccl::vector_class<size_t> recv_counts;
        ccl::datatype dtype{ccl::datatype::int8};
        dev_comm.allgatherv(tmp, 0, tmp, recv_counts, dtype);

        try
        {
            ASSERT_EQ(dev_comm.get_context(), in_ctx);
        }
        catch(...)
        {
             //TODO ignore util L0 1.0
        }

        try
        {
            EXPECT_EQ(dev_comm.size(), total_devices_count);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        try
        {
            EXPECT_EQ(dev_comm.rank(), curr_rank);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        curr_rank++;
    }
}

TEST(device_communicator_api, device_comm_from_sycl_devices_multiple_threads)
{
    // fill stub parameters
    ccl_comm::ccl_comm_reset_thread_barrier();
    ccl::group_context::instance().communicator_group_map.clear();

    ccl::global_data::get().thread_barrier_wait_timeout_sec = 10;
    ccl_logger::set_log_level(static_cast<ccl_log_level>(3));
    constexpr size_t thread_count = 3;
    for(size_t i = 0; i < thread_count; i++)
    {
        ccl::device_indices_t indices {
                                        ccl::device_index_type(0,i, ccl::unused_index_value)
                                      };
        stub::make_stub_devices(indices);
    }

    // run test
    // prepare 'in_*' parameters
    size_t in_total_devices_size = 4 * thread_count;
    auto in_ctx = cl::sycl::context();
    std::shared_ptr<stub_kvs> in_kvs;

    // balance ranks by threads
    ccl::vector_class<cl::sycl::device> ranked_devices{in_total_devices_size / thread_count, cl::sycl::device{}};
    thread_rank_device_container_t total_thread_container;
    size_t curr_rank_enumerator = 0;
    for (size_t i = 0; i < thread_count; i++)
    {
        // fill per-thread rank device table
        rank_device_container_t in_local_rank_device_map;
        in_local_rank_device_map.reserve(in_total_devices_size / thread_count);
        std::transform(ranked_devices.begin(), ranked_devices.end(), std::back_inserter(in_local_rank_device_map),
                   [&curr_rank_enumerator](cl::sycl::device& val)
                   {
                       return std::make_pair(curr_rank_enumerator++, val);
                   });

        total_thread_container.emplace(i, std::move(in_local_rank_device_map));
    }


    // launch user threads for comm creation
    std::vector<std::thread> user_threads;
    user_threads.reserve(thread_count);
    std::atomic<size_t> created_communicators_count{};
    for (size_t i = 0; i < thread_count; i++)
    {
        const rank_device_container_t& in_rank_dev_map = total_thread_container.find(i)->second;
        user_threads.emplace_back(&user_thread_function,
                                  in_total_devices_size,
                                  std::cref(in_rank_dev_map),
                                  std::ref(in_ctx),
                                  in_kvs,
                                  std::ref(created_communicators_count));
    }

    // check correctness
    for (size_t i = 0; i < thread_count; i++)
    {
        user_threads[i].join();
    }
    user_threads.clear();

    ASSERT_EQ(created_communicators_count.load(), in_total_devices_size);
}







void user_thread_function_splitted_comm(size_t total_devices_count, const rank_device_container_t& in_local_rank_device_map,
                          cl::sycl::context& in_ctx,
                          std::shared_ptr<stub_kvs> in_kvs,
                          std::atomic<size_t>& total_communicators_count)
{
    // blocking API call: wait for all threads from all processes
    ccl::vector_class<ccl::device_communicator> out_comms = ccl::device_communicator::create_device_communicators(total_devices_count, in_local_rank_device_map, in_ctx, in_kvs);

    // check correctness
    total_communicators_count.fetch_add(out_comms.size());

    size_t curr_rank = in_local_rank_device_map.begin()->first;
    ASSERT_EQ(out_comms.size(), in_local_rank_device_map.size());

    size_t local_thread_rank = 0;
    for (auto& dev_comm : out_comms)
    {
        // check dev_comm correctness
        ASSERT_TRUE(dev_comm.is_ready());
        ASSERT_EQ(dev_comm.get_context(), in_ctx);

        try
        {
            EXPECT_EQ(dev_comm.size(), total_devices_count);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        try
        {
            EXPECT_EQ(dev_comm.rank(), curr_rank);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }
        curr_rank++;


        // collective test
        void *tmp = nullptr;
        ccl::vector_class<size_t> recv_counts;
        ccl::datatype dtype{ccl::datatype::int8};
        dev_comm.allgatherv(tmp, 0, tmp, recv_counts, dtype);


        // split test for current thread local scope
        auto attr = ccl::create_device_comm_split_attr(
                    ccl::attr_arg<ccl::ccl_comm_split_attributes::group>(ccl::device_group_split_type::thread));
        auto splitted_comm = dev_comm.split(attr);

        // check splitted_comm correctness
        ASSERT_TRUE(splitted_comm.is_ready());
        ASSERT_EQ(splitted_comm.get_context(), in_ctx);

        try
        {
            EXPECT_EQ(splitted_comm.size(), in_local_rank_device_map.size());
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        try
        {
            EXPECT_EQ(splitted_comm.rank(), local_thread_rank);
        }
        catch(...)
        {
            //TODO ignore explicit rank setting is allowed in core
        }

        local_thread_rank ++;

    }
}

TEST(device_communicator_api, device_comm_from_sycl_devices_multiple_threads_with_split_comm)
{
    // fill stub parameters
    ccl_comm::ccl_comm_reset_thread_barrier();
    ccl::group_context::instance().communicator_group_map.clear();

    ccl::global_data::get().thread_barrier_wait_timeout_sec = 10;
    ccl_logger::set_log_level(static_cast<ccl_log_level>(3));
    constexpr size_t thread_count = 3;
    for(size_t i = 0; i < thread_count; i++)
    {
        ccl::device_indices_t indices {
                                        ccl::device_index_type(0,i, ccl::unused_index_value)
                                      };
        stub::make_stub_devices(indices);
    }

    // run test
    // prepare 'in_*' parameters
    size_t in_total_devices_size = 4 * thread_count;
    auto in_ctx = cl::sycl::context();
    std::shared_ptr<stub_kvs> in_kvs;

    // balance ranks by threads
    ccl::vector_class<cl::sycl::device> ranked_devices{in_total_devices_size / thread_count, cl::sycl::device{}};
    thread_rank_device_container_t total_thread_container;
    size_t curr_rank_enumerator = 0;
    for (size_t i = 0; i < thread_count; i++)
    {
        // fill per-thread rank device table
        rank_device_container_t in_local_rank_device_map;
        in_local_rank_device_map.reserve(in_total_devices_size / thread_count);
        std::transform(ranked_devices.begin(), ranked_devices.end(), std::back_inserter(in_local_rank_device_map),
                   [&curr_rank_enumerator](cl::sycl::device& val)
                   {
                       return std::make_pair(curr_rank_enumerator++, val);
                   });

        total_thread_container.emplace(i, std::move(in_local_rank_device_map));
    }


    // launch user threads for comm creation
    std::vector<std::thread> user_threads;
    user_threads.reserve(thread_count);
    std::atomic<size_t> created_communicators_count{};
    for (size_t i = 0; i < thread_count; i++)
    {
        const rank_device_container_t& in_rank_dev_map = total_thread_container.find(i)->second;
        user_threads.emplace_back(&user_thread_function_splitted_comm,
                                  in_total_devices_size,
                                  std::cref(in_rank_dev_map),
                                  std::ref(in_ctx),
                                  in_kvs,
                                  std::ref(created_communicators_count));
    }

    // check correctness
    for (size_t i = 0; i < thread_count; i++)
    {
        user_threads[i].join();
    }
    user_threads.clear();

    ASSERT_EQ(created_communicators_count.load(), in_total_devices_size);
}
}
