#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_context_attr_ids.hpp"
#include "oneapi/ccl/ccl_context_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_context.hpp"

#include "context_impl.hpp"

namespace context_suite {

TEST(context_api, device_dontext_api_from_sycl_context_creation) {
    auto dev = cl::sycl::device();
    auto ctx = cl::sycl::context(dev);
    auto str = ccl::v1::context::create_context(ctx);

    ASSERT_TRUE(str.get<ccl::v1::context_attr_id::version>().full != nullptr);
    ASSERT_EQ(str.get_native(), ctx);
}

TEST(context_api, context_api_from_sycl_context_attr_creation) {
    auto ctx = cl::sycl::context();
    cl_context h = ctx.get();
    auto str = ccl::v1::context::create_context_from_attr(h);

    ASSERT_TRUE(str.get<ccl::v1::context_attr_id::version>().full != nullptr);
}

} // namespace context_suite
#undef protected
#undef private
