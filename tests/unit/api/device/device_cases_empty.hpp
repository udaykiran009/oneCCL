#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_device_attr_ids.hpp"
#include "oneapi/ccl/ccl_device_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_device.hpp"

#include "device_impl.hpp"

namespace device_suite {

TEST(device_api, device_from_empty) {

    static typename ccl::unified_device_type::handle_t default_native_device;

    auto str = ccl::device::create_device(default_native_device);
    ASSERT_TRUE(str.get<ccl::device_attr_id::version>().full != nullptr);
    //ASSERT_EQ(str.get_native(), default_native_device);
}
}
