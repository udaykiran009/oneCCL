#pragma once
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"


class ccl_allgather_op_attr_impl_t : public ccl_common_op_attr_impl_t
{
    using base_t = ccl_common_op_attr_impl_t;
    using base_t::comm_attr;

    ccl_allgather_op_attr_impl_t(const base_t& base, const ccl_version_t& lib_version);
    ccl_allgather_op_attr_impl_t(const ccl_allgather_op_attr_impl_t& src);

    constexpr static int color_default()
    {
        return 0;
    }
vector_buf_id
    int set_attribute_value(int preferred_color);
    ccl_version_t set_attribute_value(ccl_version_t);

    const int& get_attribute_value(std::integral_constant<allgatherv_op_attr_id,
                                                   allgatherv_op_attr_id::ccl_host_color> stub) const;
    const ccl_version_t& get_attribute_value(std::integral_constant<allgatherv_op_attr_id,
                                                   allgatherv_op_attr_id::ccl_host_version> stub) const;
private:
    ccl_version_t library_version;
};




ccl_allgather_op_attr_impl_t::ccl_allgather_op_attr_impl_t (const base_t& base, const ccl_version_t& lib_version) :
        base_t(base),
        library_version(lib_version)
{
}

ccl_allgather_op_attr_impl_t::ccl_allgather_op_attr_impl_t(const ccl_allgather_op_attr_impl_t& src) :
        base_t(src),
        library_version(src.library_version)
{
}

int ccl_allgather_op_attr_impl_t::set_attribute_value(int preferred_color)
{
    int old = comm_attr.color;
    comm_attr.color = preferred_color;
    return old;
}

ccl_version_t ccl_allgather_op_attr_impl_t::set_attribute_value(ccl_version_t)
{
    return library_version;
}

const int& ccl_allgather_op_attr_impl_t::get_attribute_value(std::integral_constant<ccl_comm_split_attributes,
                                               ccl_comm_split_attributes::ccl_host_color> stub) const
{
    return comm_attr.color;
}

const ccl_version_t& ccl_allgather_op_attr_impl_t::get_attribute_value(std::integral_constant<ccl_comm_split_attributes,
                                                         ccl_comm_split_attributes::ccl_host_version> stub) const
{
    return library_version;
}
