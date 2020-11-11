#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"

#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "oneapi/ccl/coll_attr.hpp"

#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

#include "oneapi/ccl/comm_attr_ids.hpp"
#include "oneapi/ccl/comm_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_attr.hpp"

#include "oneapi/ccl/stream_attr_ids.hpp"
#include "oneapi/ccl/stream_attr_ids_traits.hpp"
#include "oneapi/ccl/stream.hpp"

#include "oneapi/ccl/event.hpp"

#include "coll/coll_attributes.hpp"

#include "common/comm/comm_common_attr.hpp"
#include "comm_attr_impl.hpp"

#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

#include "oneapi/ccl/communicator.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "stream_impl.hpp"

#include "../stubs/kvs.hpp"

#include "common/global/global.hpp"

#include "../stubs/native_platform.hpp"

//TODO
#include "common/comm/comm.hpp"

#include "common/comm/l0/comm_context.hpp"

#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace device_communicator_suite {

TEST(device_communicator_api, device_comm_from_device_index) {
    int total_devices_size = 4;
    ccl::vector_class<ccl::device_index_type> devices( total_devices_size,
                                                       ccl::from_string("[0:6459]") );
    auto ctx = native::get_platform().get_driver(0)->create_context();
    std::shared_ptr<stub_kvs> stub_storage;

    ccl::vector_class<ccl::pair_class<int, ccl::device_index_type>> local_rank_device_map;
    local_rank_device_map.reserve(total_devices_size);
    int curr_rank = 0;
    std::transform(devices.begin(),
                   devices.end(),
                   std::back_inserter(local_rank_device_map),
                   [&curr_rank](ccl::device_index_type& val) {
                       return std::make_pair(curr_rank++, val);
                   });

    ccl::vector_class<ccl::v1::communicator> comms = ccl::v1::communicator::create_communicators(
        total_devices_size, local_rank_device_map, ctx, stub_storage);
    ASSERT_EQ(comms.size(), devices.size());

    for (const auto& dev_comm : comms) {
        //ASSERT_TRUE(dev_comm.is_ready());
        ASSERT_EQ(dev_comm.size(), total_devices_size);
    }
}
} // namespace device_communicator_suite
