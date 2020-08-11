#pragma once

//API headers with declaration of new API object
#define private public
#define protected public
#include "ccl_types.h"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

// Core file with PIMPL implementation
#include "comm_split_attr_impl.hpp"

#undef protected
#undef private

namespace host_comm_split_attr_suite
{

TEST(host_comm_split_attr, host_comm_split_attr_empty_creation)
{
    ccl::comm_split_attr_t attr = ccl::create_comm_split_attr();
    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::version>());
    ASSERT_TRUE(attr.get<ccl::ccl_comm_split_attributes::version>().full != nullptr);
    ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::color>());
    ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::group>());
}

TEST(host_comm_split_attr, host_comm_split_attr_color)
{
    auto attr = ccl::create_comm_split_attr(
                    ccl::attr_arg<ccl::ccl_comm_split_attributes::color>(123)
                );
    ASSERT_TRUE(attr.get<ccl::ccl_comm_split_attributes::version>().full != nullptr);

    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::color>());
    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::color>(), 123);

    auto old_value = attr.set<ccl::ccl_comm_split_attributes::color>(1234);
    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::color>());
    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::color>(), 1234);
    ASSERT_EQ(old_value, 123);
}

TEST(host_comm_split_attr, host_comm_split_attr_group)
{
    auto attr = ccl::create_comm_split_attr(
                    ccl::attr_arg<ccl::ccl_comm_split_attributes::group>(ccl::ccl_group_split_type::thread)
                );
    ASSERT_TRUE(attr.get<ccl::ccl_comm_split_attributes::version>().full != nullptr);

    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::group>());
    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::group>(), ccl::ccl_group_split_type::thread);

    auto old_value = attr.set<ccl::ccl_comm_split_attributes::group>(ccl::ccl_group_split_type::process);
    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::group>());
    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::group>(), ccl::ccl_group_split_type::process);
    ASSERT_EQ(old_value, ccl::ccl_group_split_type::thread);
}

TEST(host_comm_split_attr, copy_on_write_host_comm_split_attr)
{
    auto attr = ccl::create_comm_split_attr(
                    ccl::attr_arg<ccl::ccl_comm_split_attributes::group>(ccl::ccl_group_split_type::thread)
                );

    auto original_inner_impl_ptr = attr.get_impl();

    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::group>(), ccl::ccl_group_split_type::thread);

    //set new val
    attr.set<ccl::ccl_comm_split_attributes::group>(ccl::ccl_group_split_type::process);
    ASSERT_EQ(attr.get<ccl::ccl_comm_split_attributes::group>(), ccl::ccl_group_split_type::process);

    //make sure original impl is unchanged
    ASSERT_TRUE(original_inner_impl_ptr !=attr.get_impl());
    ASSERT_EQ(std::static_pointer_cast<ccl::ccl_host_comm_split_attr_impl>(original_inner_impl_ptr)->get_attribute_value(
    ccl::details::ccl_host_split_traits<ccl::ccl_comm_split_attributes, ccl::ccl_comm_split_attributes::group> {}), ccl::ccl_group_split_type::thread);
}

TEST(host_comm_split_attr, copy_host_comm_split_attr)
{
    auto attr = ccl::create_comm_split_attr();
    attr.set<ccl::ccl_comm_split_attributes::color>(666);

    auto original_inner_impl_ptr = attr.get_impl().get();
    auto attr2 = attr;
    auto copied_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_TRUE(original_inner_impl_ptr != copied_inner_impl_ptr);
    ASSERT_TRUE(attr.get_impl());
    ASSERT_TRUE(attr2.get<ccl::ccl_comm_split_attributes::version>().full != nullptr);
    ASSERT_EQ(attr2.get<ccl::ccl_comm_split_attributes::color>(), 666);
}

TEST(host_comm_split_attr, move_host_comm_split_attr)
{
    auto attr = ccl::create_comm_split_attr();
    attr.set<ccl::ccl_comm_split_attributes::color>(667);

    auto original_inner_impl_ptr = attr.get_impl().get();

    auto attr2 = (std::move(attr));
    auto moved_inner_impl_ptr = attr2.get_impl().get();
    ASSERT_EQ(original_inner_impl_ptr, moved_inner_impl_ptr);
    ASSERT_TRUE(!attr.get_impl());
    ASSERT_TRUE(attr2.get<ccl::ccl_comm_split_attributes::version>().full != nullptr);
    ASSERT_EQ(attr2.get<ccl::ccl_comm_split_attributes::color>(), 667);
}

TEST(host_comm_split_attr, host_comm_split_attr_valid)
{
    auto attr = ccl::create_comm_split_attr();

    ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::color>());
    ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::group>());

    attr.set<ccl::ccl_comm_split_attributes::group>(ccl::ccl_group_split_type::process);
    ASSERT_TRUE(attr.is_valid<ccl::ccl_comm_split_attributes::group>());
    ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::color>());

    auto attr2 = ccl::create_comm_split_attr();

    try
    {
        attr.get<ccl::ccl_comm_split_attributes::color>();
        ASSERT_TRUE(false); // must never happen
    }
    catch(...)
    {
        ASSERT_TRUE(!attr.is_valid<ccl::ccl_comm_split_attributes::color>());
    }
}

} // namespace host_comm_split_attr_suite
