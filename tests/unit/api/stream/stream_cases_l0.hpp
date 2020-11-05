#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"

#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/stream_attr_ids.hpp"
#include "oneapi/ccl/stream_attr_ids_traits.hpp"
#include "oneapi/ccl/stream.hpp"

// Core file with PIMPL implementation
#include "environment.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

namespace stream_suite {
TEST(stream_api, stream_from_device_creation) {
    auto dev = native::get_platform().get_device(ccl::from_string("[0:6459]"));
    auto str = ccl::v1::stream::create_stream_from_attr(dev);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);

    auto assigned_dev = str.get<ccl::v1::stream_attr_id::device>();
    ASSERT_EQ(assigned_dev.get(), dev.get());
}

TEST(stream_api, stream_from_context_creation) {
    auto dev = native::get_platform().get_device(ccl::from_string("[0:6459]"));
    auto ctx = native::get_platform().get_driver(0)->create_context();
    auto str = ccl::v1::stream::create_stream_from_attr(dev, ctx);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);

    auto assigned_dev = str.get<ccl::v1::stream_attr_id::device>();
    ASSERT_EQ(assigned_dev.get(), dev.get());

    auto assigned_ctx = str.get<ccl::v1::stream_attr_id::context>();
    ASSERT_EQ(assigned_ctx.get(), ctx.get());
}

TEST(stream_api, stream_creation_from_native) {
    auto dev = native::get_platform().get_device(ccl::from_string("[0:6459]"));
    auto ctx = native::get_platform().get_driver(0)->create_context();
    auto queue = dev->create_cmd_queue(ctx);

    //TODO HACK
    typename ccl::unified_stream_type::ccl_native_t *s =
        new typename ccl::unified_stream_type::ccl_native_t(&queue);
    auto str = ccl::v1::stream::create_stream(*s);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);

    auto assigned_handle = str.get<ccl::v1::stream_attr_id::native_handle>();
    ASSERT_EQ(assigned_handle, *s);
}

#if 0
TEST(stream_api, stream_creation_from_native_handle)
{
    ze_command_queue_handle_t h;
    auto str = ccl::v1::stream::create_stream(h);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);

    auto assigned_handle = str.get<ccl::v1::stream_attr_id::native_handle>();
    ASSERT_EQ(assigned_handle->get(), h);
    (void)assigned_handle;
}
#endif
} // namespace stream_suite
#undef protected
#undef private
