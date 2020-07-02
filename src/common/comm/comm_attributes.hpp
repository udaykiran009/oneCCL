#pragma once
#include "ccl.hpp"

namespace ccl
{
struct host_attr_impl : public ccl_host_comm_attr_t
{
    using base_t = ccl_host_comm_attr_t;
    using base_t::comm_attr;

    host_attr_impl (const base_t& base, const ccl_version_t& lib_version);

    host_attr_impl(const host_attr_impl& src);

    constexpr static int color_default()
    {
        return 0;
    }

    int set_attribute_value(int preferred_color);
    ccl_version_t set_attribute_value(ccl_version_t);

    const int& get_attribute_value(std::integral_constant<ccl_host_attributes,
                                                   ccl_host_attributes::ccl_host_color> stub) const;
    const ccl_version_t& get_attribute_value(std::integral_constant<ccl_host_attributes,
                                                   ccl_host_attributes::ccl_host_version> stub) const;
private:
    ccl_version_t library_version;
};



#ifdef MULTI_GPU_SUPPORT
struct device_attr_impl
{
    constexpr static device_topology_type class_default()
    {
        return device_topology_type::ring;
    }
    constexpr static device_group_split_type group_default()
    {
        return device_group_split_type::process;
    }

    device_topology_type set_attribute_value(device_topology_type preferred_topology);
    device_group_split_type set_attribute_value(device_group_split_type preferred_topology);

    const device_topology_type&
        get_attribute_value(std::integral_constant<ccl_device_attributes,
                                                   ccl_device_attributes::ccl_device_preferred_topology_class> stub) const;
    const device_group_split_type&
        get_attribute_value(std::integral_constant<ccl_device_attributes,
                                                   ccl_device_attributes::ccl_device_preferred_group> stub) const;
private:
    device_topology_type current_preferred_topology_class = class_default();
    device_group_split_type current_preferred_topology_group = group_default();
};
#endif
}
