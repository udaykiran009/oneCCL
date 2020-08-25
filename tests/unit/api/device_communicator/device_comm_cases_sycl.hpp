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
#include "stream_impl.hpp"

#include "ccl_request.hpp"

#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

#include "environment.hpp"
#include "device_communicator_impl.hpp"

#include "../stubs/kvs.hpp"


namespace device_communicator_suite
{

TEST(device_communicator_api, device_comm_from_sycl_device)
{

    size_t total_devices_size = 4;
    ccl::vector_class<cl::sycl::device> devices{total_devices_size, cl::sycl::device{}};
    auto ctx = cl::sycl::context();
    std::shared_ptr<stub_kvs> stub_storage;

    ccl::vector_class<ccl::pair_class<size_t, cl::sycl::device>> local_rank_device_map;
    local_rank_device_map.reserve(total_devices_size);
    size_t curr_rank = 0;
    std::transform(devices.begin(), devices.end(), std::back_inserter(local_rank_device_map),
                   [&curr_rank](cl::sycl::device& val)
                   {
                       return std::make_pair(curr_rank++, val);
                   });

    ccl::vector_class<ccl::device_communicator> comms = create_device_communicators(total_devices_size, local_rank_device_map, ctx, stub_storage);
    ASSERT_EQ(comms.size(), devices.size());

    for (const auto& dev_comm : comms)
    {
        ASSERT_TRUE(dev_comm.is_ready());
        ASSERT_EQ(dev_comm.size(), total_devices_size);
    }
}
}
