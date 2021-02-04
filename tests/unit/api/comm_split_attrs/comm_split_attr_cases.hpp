#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

// Core file with PIMPL implementation
#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

#undef protected
#undef private

namespace comm_split_attr_suite {

TEST(comm_split_attr, comm_split_attr_empty_creation) {
    auto attr = ccl::v1::default_comm_split_attr;
    ASSERT_TRUE(attr.is_valid<ccl::v1::comm_split_attr_id::version>());
    ASSERT_TRUE(attr.get<ccl::v1::comm_split_attr_id::version>().full != nullptr);
    ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::group>());
}

TEST(comm_split_attr, comm_split_attr_color) {
    auto attr = ccl::v1::default_comm_split_attr;
    attr.set<ccl::v1::comm_split_attr_id::color>(123);
    ASSERT_TRUE(attr.get<ccl::v1::comm_split_attr_id::version>().full != nullptr);

    ASSERT_TRUE(attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_EQ(attr.get<ccl::v1::comm_split_attr_id::color>(), 123);

    auto old_value = attr.set<ccl::v1::comm_split_attr_id::color>(1234);
    ASSERT_TRUE(attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_EQ(attr.get<ccl::v1::comm_split_attr_id::color>(), 1234);
    ASSERT_EQ(old_value, 123);
}

TEST(comm_split_attr, comm_split_attr_group) {
    auto attr = ccl::v1::default_comm_split_attr;
    attr.set<ccl::v1::comm_split_attr_id::group>(ccl::v1::split_group::cluster);
    ASSERT_TRUE(attr.get<ccl::v1::comm_split_attr_id::version>().full != nullptr);

    ASSERT_TRUE(attr.is_valid<ccl::v1::comm_split_attr_id::group>());
    ASSERT_EQ(attr.get<ccl::v1::comm_split_attr_id::group>(), ccl::v1::split_group::cluster);
}

TEST(comm_split_attr, copy_on_write_comm_split_attr) {
    auto attr = ccl::v1::default_comm_split_attr;
    attr.set<ccl::v1::comm_split_attr_id::color>(1);

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get<ccl::v1::comm_split_attr_id::color>(), 1);

    //set new val
    attr.set<ccl::v1::comm_split_attr_id::color>(2);
    ASSERT_EQ(attr.get<ccl::v1::comm_split_attr_id::color>(), 2);

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr != attr.get_impl());
    ASSERT_EQ(std::static_pointer_cast<ccl::ccl_comm_split_attr_impl>(original_inner_impl_ptr)
                  ->get_attribute_value(
                      ccl::detail::ccl_api_type_attr_traits<ccl::v1::comm_split_attr_id,
                                                            ccl::v1::comm_split_attr_id::color>{}),
              1);
}

TEST(comm_split_attr, copy_comm_split_attr) {
    auto attr = ccl::v1::default_comm_split_attr;
    attr.set<ccl::v1::comm_split_attr_id::color>(123);

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
    ASSERT_TRUE(attr2.get<ccl::v1::comm_split_attr_id::version>().full != nullptr);
    ASSERT_EQ(attr2.get<ccl::v1::comm_split_attr_id::color>(), 123);
}

TEST(comm_split_attr, move_comm_split_attr) {
    /* move constructor test */
    auto orig_attr = ccl::v1::default_comm_split_attr;
    orig_attr.set<ccl::v1::comm_split_attr_id::color>(124);

    auto orig_inner_impl_ptr = orig_attr.get_impl().get();
    auto moved_attr = (std::move(orig_attr));
    auto moved_inner_impl_ptr = moved_attr.get_impl().get();

    ASSERT_EQ(orig_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(moved_attr.get_impl());
    ASSERT_TRUE(!orig_attr.get_impl());
    ASSERT_TRUE(moved_attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_EQ(moved_attr.get<ccl::v1::comm_split_attr_id::color>(), 124);

    /* move assignment test*/
    auto orig_attr2 = ccl::v1::default_comm_split_attr;
    orig_attr2.set<ccl::v1::comm_split_attr_id::color>(123);

    auto moved_attr2 = ccl::v1::default_comm_split_attr;
    moved_attr2 = std::move(orig_attr2);

    ASSERT_TRUE(moved_attr2.get_impl());
    ASSERT_TRUE(!orig_attr2.get_impl());
    ASSERT_TRUE(moved_attr2.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_EQ(moved_attr2.get<ccl::v1::comm_split_attr_id::color>(), 123);
}

TEST(comm_split_attr, comm_split_attr_valid) {
    auto attr = ccl::v1::default_comm_split_attr;

    ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::group>());

    attr.set<ccl::v1::comm_split_attr_id::group>(ccl::v1::split_group::cluster);
    ASSERT_TRUE(attr.is_valid<ccl::v1::comm_split_attr_id::group>());
    ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::color>());

    auto attr2 = ccl::v1::default_comm_split_attr;

    try {
        attr.get<ccl::v1::comm_split_attr_id::color>();
        ASSERT_TRUE(false); // must never happen
    }
    catch (...) {
        ASSERT_TRUE(!attr.is_valid<ccl::v1::comm_split_attr_id::color>());
    }
}

} // namespace comm_split_attr_suite
