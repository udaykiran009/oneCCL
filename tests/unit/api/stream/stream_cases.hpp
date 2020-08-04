#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

// Core file with PIMPL implementation
#ifndef DEVICE_COMM_SUPPORT
    #define DEVICE_COMM_SUPPORT
#endif

#include "environment.hpp"
#include "native_device_api/export_api.hpp"

namespace stream_suite
{
TEST(stream_api, stream_creation_from_native)
{
    ze_command_queue_handle_t h;
    auto str = ccl::create_stream(h);

    ASSERT_TRUE(str.get<ccl::stream_attr_id::version>().full != nullptr);

    auto assigned_handle = str.get<ccl::stream_attr_id::native_handle>();
    ASSERT_EQ(assigned_handle, h);
}

TEST(stream_api, stream_from_device_creation)
{
    auto dev = native::get_platform().get_device(ccl::from_string("[0:6459]"));
    auto str = ccl::create_stream_from_attr(dev);

    ASSERT_TRUE(str.get<ccl::stream_attr_id::version>().full != nullptr);

    auto assigned_dev = str.get<ccl::stream_attr_id::device>();
    ASSERT_EQ(assigned_dev.get(), dev.get());
}

TEST(stream_api, stream_from_device_context_creation)
{
    auto dev = native::get_platform().get_device(ccl::from_string("[0:6459]"));
    auto ctx = std::make_shared<native::ccl_context>(); //TODO stub at moment
    auto str = ccl::create_stream_from_attr(dev, ctx);

    ASSERT_TRUE(str.get<ccl::stream_attr_id::version>().full != nullptr);

    auto assigned_dev = str.get<ccl::stream_attr_id::device>();
    ASSERT_EQ(assigned_dev.get(), dev.get());

    auto assigned_ctx = str.get<ccl::stream_attr_id::context>();
    ASSERT_EQ(assigned_ctx.get(), ctx.get());
}
}
#undef protected
#undef private
