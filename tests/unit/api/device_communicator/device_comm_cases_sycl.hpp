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

#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"
#include "stream_impl.hpp"

#include "ccl_request.hpp"

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
    ccl::vector_class<ccl::device_communicator> comms = create_device_communicators(total_devices_size, devices, ctx, stub_storage);
    (void)comms;
}
}
