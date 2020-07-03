#include "common/comm/comm_attributes.hpp"

namespace ccl {

host_attr_impl::host_attr_impl(const base_t& base, const ccl_version_t& lib_version)
        : base_t(base),
          library_version(lib_version) {}

host_attr_impl::host_attr_impl(const host_attr_impl& src)
        : base_t(src),
          library_version(src.library_version) {}

int host_attr_impl::set_attribute_value(int preferred_color) {
    int old = comm_attr.color;
    comm_attr.color = preferred_color;
    return old;
}

ccl_version_t host_attr_impl::set_attribute_value(ccl_version_t) {
    return library_version;
}

const int& host_attr_impl::get_attribute_value(
    std::integral_constant<ccl_host_attributes, ccl_host_attributes::ccl_host_color> stub) const {
    return comm_attr.color;
}

const ccl_version_t& host_attr_impl::get_attribute_value(
    std::integral_constant<ccl_host_attributes, ccl_host_attributes::ccl_host_version> stub) const {
    return library_version;
}

#ifdef MULTI_GPU_SUPPORT
device_topology_type device_attr_impl::set_attribute_value(
    device_topology_type preferred_topology) {
    device_topology_type old = current_preferred_topology_class;
    current_preferred_topology_class = preferred_topology;
    return old;
}

device_group_split_type device_attr_impl::set_attribute_value(
    device_group_split_type preferred_topology) {
    device_group_split_type old = current_preferred_topology_group;
    current_preferred_topology_group = preferred_topology;
    return old;
}

const device_topology_type& device_attr_impl::get_attribute_value(
    std::integral_constant<ccl_device_attributes,
                           ccl_device_attributes::ccl_device_preferred_topology_class> stub) const {
    return current_preferred_topology_class;
}
const device_group_split_type& device_attr_impl::get_attribute_value(
    std::integral_constant<ccl_device_attributes, ccl_device_attributes::ccl_device_preferred_group>
        stub) const {
    return current_preferred_topology_group;
}
#endif
} // namespace ccl
