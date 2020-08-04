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

#include "environment.hpp"


namespace stream_suite
{

TEST(stream_api, stream_from_sycl_device_creation)
{
    auto dev = cl::sycl::device();
    auto str = ccl::create_stream_from_attr(dev);

    ASSERT_TRUE(str.get<ccl::stream_attr_id::version>().full != nullptr);
}

TEST(stream_api, stream_from_sycl_device_context_creation)
{
    auto dev = cl::sycl::device();
    auto ctx = cl::sycl::context();
    auto str = ccl::create_stream_from_attr(dev, ctx);

    ASSERT_TRUE(str.get<ccl::stream_attr_id::version>().full != nullptr);
}

}
#undef protected
#undef private
