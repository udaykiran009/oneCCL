#include "common/comm/compiler_comm_interface_dispatcher.hpp"

#include "common/comm/comm_interface.hpp"

#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

#include "common/comm/comm_common_attr.hpp"
#include "common/comm/comm_impl.hpp"
#include "common/comm/comm_split_common_attr.hpp"
#include "common/global/global.hpp"

#include "comm_attr_impl.hpp"
#include "comm_split_attr_impl.hpp"

namespace ccl {

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl() {
    return communicator_interface_ptr(new ccl_comm());
}

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const size_t size,
    shared_ptr_class<ikvs_wrapper> kvs) {
    return communicator_interface_ptr(new ccl_comm(size, kvs));
}

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const size_t size,
    const int rank,
    shared_ptr_class<ikvs_wrapper> kvs) {
    return communicator_interface_ptr(new ccl_comm(size, rank, kvs));
}

template <class DeviceType,
          class ContextType,
          typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                  int>::type>
communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const DeviceType& device,
    ContextType context,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr,
    std::shared_ptr<atl_base_comm> atl) {
    static_assert(std::is_same<typename unified_device_type::ccl_native_t, DeviceType>::value,
                  "Unsupported 'DeviceType'");

    return communicator_interface_dispatcher::create_communicator_from_unified_device(
        unified_device_type(device),
        unified_context_type(context),
        thread_idx,
        process_idx,
        attr,
        atl);
}

template <class DeviceType,
          class ContextType,
          typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                  int>::type>
communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    DeviceType device_id,
    ContextType context,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr,
    std::shared_ptr<atl_base_comm> atl) {
#ifdef CCL_ENABLE_SYCL
    return communicator_interface_dispatcher::create_communicator_from_unified_device(
        unified_device_type(device_id, cl::sycl::info::device_type::gpu),
        unified_context_type(context),
        thread_idx,
        process_idx,
        attr,
        atl);
#else
    return communicator_interface_dispatcher::create_communicator_from_unified_device(
        unified_device_type(device_id),
        unified_context_type(context),
        thread_idx,
        process_idx,
        attr,
        atl);
#endif
}

communicator_interface_ptr
communicator_interface_dispatcher::create_communicator_from_unified_device(
    ccl::unified_device_type&& device_id,
    ccl::unified_context_type&& context,
    size_t thread_idx,
    size_t process_idx,
    const ccl::comm_split_attr& attr,
    std::shared_ptr<atl_base_comm> atl) {
    if (attr.is_valid<ccl::comm_split_attr_id::group>()) {
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for 'group'");
        if (attr.is_valid<ccl::comm_split_attr_id::color>()) {
            throw ccl::exception(std::string(
                "invalid `comm_split_attr`: both `color` and `group` set, only one is supported"));
        }
    }
    else if (attr.is_valid<ccl::comm_split_attr_id::color>()) {
        throw ccl::exception(std::string(__FUNCTION__) + " - not implemented for 'color'");
    }

    return communicator_interface_ptr(new ccl_comm(std::move(device_id), std::move(context), atl));
}

#define COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(DeviceType, ContextType) \
    template ccl::communicator_interface_ptr \
    ccl::communicator_interface_dispatcher::create_communicator_impl( \
        const DeviceType& device, \
        ContextType context, \
        size_t thread_idx, \
        size_t process_idx, \
        const ccl::comm_split_attr& attr, \
        std::shared_ptr<atl_base_comm> atl);

#define COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(DeviceType, \
                                                                           ContextType) \
    template ccl::communicator_interface_ptr \
    ccl::communicator_interface_dispatcher::create_communicator_impl( \
        DeviceType device_id, \
        ContextType context, \
        size_t thread_idx, \
        size_t process_idx, \
        const ccl::comm_split_attr& attr, \
        std::shared_ptr<atl_base_comm> atl);

COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(
    typename ccl::unified_device_type::ccl_native_t,
    typename ccl::unified_context_type::ccl_native_t);
COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    typename ccl::unified_context_type::ccl_native_t);

} // namespace ccl
