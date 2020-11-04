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

TEST(context_api, context_from_empty) {
    static typename ccl::unified_context_type::ccl_native_t default_native_context;

    auto str = ccl::v1::context::create_context(default_native_context);
    ASSERT_TRUE(str.get<ccl::v1::context_attr_id::version>().full != nullptr);
    //ASSERT_EQ(str.get_native(), default_native_context);
}
} // namespace context_suite
