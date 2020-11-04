#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "environment.hpp"

namespace stream_suite {

TEST(stream_api, stream_from_sycl_queue) {
    auto q = cl::sycl::queue();
    auto str = ccl::v1::stream::create_stream(q);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);
}

TEST(stream_api, stream_from_sycl_queue_handle) {
    //auto q = cl::sycl::queue();
    auto dev = cl::sycl::device();
    auto ctx = cl::sycl::context(dev);
    //cl_command_queue h = q.get();

    auto str = ccl::v1::stream::create_stream(dev, ctx);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);
}
TEST(stream_api, stream_from_sycl_device_creation) {
    auto dev = cl::sycl::device();
    auto str = ccl::v1::stream::create_stream_from_attr(dev);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);
}

TEST(stream_api, stream_from_sycl_context_creation) {
    auto dev = cl::sycl::device();
    auto ctx = cl::sycl::context(dev);
    auto str = ccl::v1::stream::create_stream_from_attr(dev, ctx);

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);
}

TEST(stream_api, stream_from_sycl_context_creation_with_attr) {
    auto dev = cl::sycl::device();
    auto ctx = cl::sycl::context(dev);
    auto str = ccl::v1::stream::create_stream_from_attr(
        dev,
        ctx,
        ccl::v1::attr_val<ccl::v1::stream_attr_id::ordinal>(1),
        ccl::v1::attr_val<ccl::v1::stream_attr_id::priority>(100));

    ASSERT_TRUE(str.get<ccl::v1::stream_attr_id::version>().full != nullptr);

    ASSERT_EQ(str.get<ccl::v1::stream_attr_id::ordinal>(), 1);
    ASSERT_EQ(str.get<ccl::v1::stream_attr_id::priority>(), 100);

    bool catched = false;
    try {
        str.set<ccl::v1::stream_attr_id::priority>(99);
    }
    catch (const ccl::exception& ex) {
        catched = true;
    }
    ASSERT_TRUE(catched);
}
} // namespace stream_suite
#undef protected
#undef private
