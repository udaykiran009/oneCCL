#pragma once
#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"
#include "oneapi/ccl/ccl_communicator.hpp"

#include "common/comm/l0/comm_context_id.hpp"
//TODO
/*
namespace ccl
{
struct comm_split_attr_impl
{
    constexpr static int color_default()
    {
        return 0;
    }
    ccl::library_version version;
};

struct device_attr_impl
{
    constexpr static device_topology_type class_default()
    {
        return device_topology_type::ring;
    }
    constexpr static group_split_type group_default()
    {
        return group_split_type::process;
    }
    device_topology_type current_preferred_topology_class = class_default();
    group_split_type current_preferred_topology_group = group_default();
};
}*/
#include "common/comm/comm_interface.hpp"

namespace ccl {
/* TODO temporary function for UT compilation: would be part of ccl::environment in final
template <class event_type,
          class ...attr_value_pair_t>
event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps)
{
    ccl::library_version ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    event str {event::impl_value_t(new event::impl_t(native_event_handle, context, ret))};
    int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    str.build_from_params();

    return str;
}
*/

template <class DeviceType, class ContextType>
CCL_API vector_class<communicator> communicator::create_device_communicators(
    const size_t cluster_devices_size,
    const vector_class<DeviceType>& local_devices,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs) {
    vector_class<communicator> ret;
    throw std::runtime_error(std::string(__FUNCTION__) + " - not implemented");
    return ret;
}

using rank_t = size_t;

template <class DeviceType, class ContextType>
CCL_API vector_class<communicator> communicator::create_device_communicators(
    const size_t cluster_devices_size, /*global devics count*/
    const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs) {
#ifdef MULTI_GPU_SUPPORT
    vector_class<rank_t> local_thread_ranks;
    local_thread_ranks.reserve(local_rank_device_map.size());
    std::transform(
        local_rank_device_map.begin(),
        local_rank_device_map.end(),
        std::back_inserter(local_thread_ranks),
        [](const typename vector_class<pair_class<rank_t, DeviceType>>::value_type& val) {
            return val.first;
        });
    group_context::comm_group_t thread_group =
        group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);

    vector_class<DeviceType> local_thread_devices;
    local_thread_devices.reserve(local_rank_device_map.size());
    std::transform(
        local_rank_device_map.begin(),
        local_rank_device_map.end(),
        std::back_inserter(local_thread_devices),
        [](const typename vector_class<pair_class<rank_t, DeviceType>>::value_type& val) {
            return val.second;
        });

    auto ret = thread_group->create_communicators(local_thread_devices);
    return ret;
#endif
    return {};
}

template <class DeviceType, class ContextType>
CCL_API vector_class<communicator> communicator::create_device_communicators(
    const size_t cluster_devices_size, /*global devics count*/
    const map_class<rank_t, DeviceType>& local_rank_device_map,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs)

{
#ifdef MULTI_GPU_SUPPORT
    vector_class<rank_t> local_thread_ranks;
    local_thread_ranks.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_ranks),
                   [](const typename map_class<rank_t, DeviceType>::value_type& val) {
                       return val.first;
                   });
    group_context::comm_group_t thread_group =
        group_context::instance().group_by_kvs(local_thread_ranks, cluster_devices_size, kvs);

    vector_class<DeviceType> local_thread_devices;
    local_thread_devices.reserve(local_rank_device_map.size());
    std::transform(local_rank_device_map.begin(),
                   local_rank_device_map.end(),
                   std::back_inserter(local_thread_devices),
                   [](const typename map_class<rank_t, DeviceType>::value_type& val) {
                       return val.second;
                   });

    auto ret = thread_group->create_communicators(local_thread_devices);
    return ret;
#endif
    return {};
}

/*CCL_API bool ccl::communicator::is_ready() const
{
    return get_impl()->is_ready();
}*/

/**
 * Creates a new host communicator with externally provided size, rank and kvs.
 * Implementation is platform specific and non portable.
 * @return host communicator
 */
communicator communicator::create_communicator() {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");

    LOG_TRACE("Create host communicator");

    ccl::communicator_interface_ptr impl =
        ccl::communicator_interface::create_communicator_impl();

    return ccl::communicator(std::move(impl));
}

/**
 * Creates a new host communicator with user supplied size and kvs.
 * Rank will be assigned automatically.
 * @param size user-supplied total number of ranks
 * @param kvs key-value store for ranks wire-up
 * @return host communicator
 */
communicator communicator::create_communicator(const size_t size,
                                               shared_ptr_class<kvs_interface> kvs) {
    throw ccl::exception(std::string(__PRETTY_FUNCTION__) + " - is not implemented");

    LOG_TRACE("Create host communicator");

    ccl::communicator_interface_ptr impl =
        ccl::communicator_interface::create_communicator_impl(size, kvs);

    return ccl::communicator(std::move(impl));
}

/**
 * Creates a new host communicator with user supplied size, rank and kvs.
 * @param size user-supplied total number of ranks
 * @param rank user-supplied rank
 * @param kvs key-value store for ranks wire-up
 * @return host communicator
 */
communicator communicator::create_communicator(const size_t size,
                                               const size_t rank,
                                               shared_ptr_class<kvs_interface> kvs) {
    LOG_TRACE("Create host communicator");

    ccl::communicator_interface_ptr impl =
        ccl::communicator_interface::create_communicator_impl(size, rank, kvs);

    return ccl::communicator(std::move(impl));
}

} // namespace ccl

/***************************TypeGenerations*********************************************************/
#define API_DEVICE_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(DeviceType, ContextType) \
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::communicator::create_device_communicators( \
        const size_t comm_size, \
        const ccl::vector_class<DeviceType>& local_devices, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs);

#define API_DEVICE_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(DeviceType, ContextType) \
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::communicator::create_device_communicators( \
        const size_t comm_size, \
        const ccl::vector_class<ccl::pair_class<ccl::rank_t, DeviceType>>& local_rank_device_map, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs);

#define API_DEVICE_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(DeviceType, ContextType) \
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::communicator::create_device_communicators( \
        const size_t comm_size, \
        const ccl::map_class<ccl::rank_t, DeviceType>& local_rank_device_map, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs);
