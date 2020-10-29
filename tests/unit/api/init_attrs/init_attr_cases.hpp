#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_init_attr_ids.hpp"
#include "oneapi/ccl/ccl_init_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_init_attr.hpp"

// Core file with PIMPL implementation
#include "init_attr_impl.hpp"

#undef protected
#undef private

namespace init_attr_suite {

TEST(init_attr, init_attr_empty_creation) {
    ccl::v1::init_attr attr = ccl::v1::default_init_attr;
    ASSERT_TRUE(attr.get<ccl::v1::init_attr_id::version>().full != nullptr);
}

TEST(init_attr, copy_init_attr) {
    auto attr = ccl::v1::default_init_attr;

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
    ASSERT_TRUE(attr2.get<ccl::v1::init_attr_id::version>().full != nullptr);
}

TEST(init_attr, move_init_attr) {
    /* move constructor test */
    auto orig_attr = ccl::v1::default_init_attr;

    auto orig_inner_impl_ptr = orig_attr.get_impl().get();
    auto moved_attr = (std::move(orig_attr));
    auto moved_inner_impl_ptr = moved_attr.get_impl().get();

    ASSERT_EQ(orig_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(!orig_attr.get_impl());

    /* move assignment test */
    auto orig_attr2 = ccl::v1::default_init_attr;

    auto moved_attr2 = ccl::v1::default_init_attr;
    moved_attr2 = std::move(orig_attr2);

    ASSERT_TRUE(orig_attr2.get_impl().get() == nullptr);
    ASSERT_TRUE(moved_attr2.get_impl().get() != orig_attr2.get_impl().get());
}


} // namespace init_attr_suite
