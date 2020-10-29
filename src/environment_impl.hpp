#pragma once
#include "oneapi/ccl/ccl_environment.hpp"

#include "coll/coll_attributes.hpp"

#include "common/comm/comm_common_attr.hpp"
#include "comm_attr_impl.hpp"

#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

#include "common/comm/l0/comm_context_storage.hpp"

#include "stream_impl.hpp"

#include "common/global/global.hpp"
#include "common/comm/comm.hpp"

#include "oneapi/ccl/ccl_communicator.hpp"

#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "common/utils/version.hpp"

#include "internal_types.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

#define CCL_CHECK_AND_THROW(result, diagnostic) \
    do { \
        if (result != ccl::status::success) { \
            throw ccl::exception(diagnostic); \
        } \
    } while (0);

namespace ccl {

namespace detail {

/******************** DEVICE ********************/

template <class native_device_type, typename T>
device CCL_API environment::create_device(native_device_type&& native_device) const {
    return device::create_device(std::forward<native_device_type>(native_device));
}


/******************** CONTEXT ********************/

template <class native_device_contex_type, typename T>
context CCL_API environment::create_context(native_device_contex_type&& native_context) const {
    return context::create_context(std::forward<native_device_contex_type>(native_context));
}


/******************** STREAM ********************/

template <class native_stream_type, typename T>
stream CCL_API environment::create_stream(native_stream_type& native_stream) {
    return stream::create_stream(native_stream);
}

template <class native_stream_type, class native_context_type, typename T>
stream CCL_API environment::create_stream(native_stream_type& native_stream,
                                          native_context_type& native_ctx) {
    return stream::create_stream(native_stream, native_ctx);
}


/******************** COMMUNICATOR ********************/

template <class DeviceType, class ContextType>
vector_class<communicator> CCL_API
environment::create_communicators(const size_t devices_size,
                                         const vector_class<DeviceType>& local_devices,
                                         ContextType& context,
                                         shared_ptr_class<kvs_interface> kvs,
                                         const comm_attr& attr) const {
    return communicator::create_communicators(
        devices_size, local_devices, context, kvs);
}

template <class DeviceType, class ContextType>
vector_class<communicator> CCL_API environment::create_communicators(
    const size_t comm_size,
    const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
    ContextType& context,
    shared_ptr_class<kvs_interface> kvs,
    const comm_attr& attr) const {

    return communicator::create_communicators(
        comm_size, local_rank_device_map, context, kvs);
/*
    (void)context;
    vector_class<communicator> ret;
    ret.push_back(create_single_device_communicator(comm_size,
                                                    local_rank_device_map.begin()->first,
                                                    local_rank_device_map.begin()->second,
                                                    context,
                                                    kvs));
    return ret;
*/
}

template <class DeviceType, class ContextType>
vector_class<communicator> CCL_API
environment::create_communicators(const size_t comm_size,
                                         const map_class<rank_t, DeviceType>& local_rank_device_map,
                                         ContextType& context,
                                         shared_ptr_class<kvs_interface> kvs,
                                         const comm_attr& attr) const {
    return communicator::create_communicators(
        comm_size, local_rank_device_map, context, kvs);
/*
    (void)context;
    vector_class<communicator> ret;
    ret.push_back(create_single_device_communicator(comm_size,
                                                    local_rank_device_map.begin()->first,
                                                    local_rank_device_map.begin()->second,
                                                    context,
                                                    kvs));
    return ret;
*/
}

} // namespace detail

} // namespace ccl


/******************** TypeGenerations ********************/

#define CREATE_DEV_COMM_INSTANTIATION(DeviceType, ContextType) \
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::detail::environment::create_communicators<DeviceType, ContextType>( \
        const size_t devices_size, \
        const ccl::vector_class<DeviceType>& local_devices, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs, \
        const comm_attr& attr) const; \
\
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::detail::environment::create_communicators<DeviceType, ContextType>( \
        const size_t cluster_devices_size, \
        const ccl::vector_class<ccl::pair_class<ccl::rank_t, DeviceType>>& local_devices, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs, \
        const comm_attr& attr) const; \
\
    template ccl::vector_class<ccl::communicator> CCL_API \
    ccl::detail::environment::create_communicators<DeviceType, ContextType>( \
        const size_t cluster_devices_size, \
        const ccl::map_class<ccl::rank_t, DeviceType>& local_devices, \
        ContextType& context, \
        ccl::shared_ptr_class<ccl::kvs_interface> kvs, \
        const comm_attr& attr) const;

#define CREATE_STREAM_INSTANTIATION(native_stream_type) \
    template ccl::stream CCL_API ccl::detail::environment::create_stream(native_stream_type& native_stream);

#define CREATE_STREAM_EXT_INSTANTIATION(device_type, native_context_type) \
    template ccl::stream CCL_API ccl::detail::environment::create_stream(device_type& device, \
                                                                 native_context_type& native_ctx);

#define CREATE_CONTEXT_INSTANTIATION(native_context_type) \
    template ccl::context CCL_API ccl::detail::environment::create_context(native_context_type&& native_ctx) const; \
    template ccl::context CCL_API ccl::detail::environment::create_context(native_context_type& native_ctx) const; \
    template ccl::context CCL_API ccl::detail::environment::create_context(const native_context_type& native_ctx) const;

#define CREATE_DEVICE_INSTANTIATION(native_device_type) \
    template ccl::device CCL_API ccl::detail::environment::create_device(native_device_type&& native_device) const; \
    template ccl::device CCL_API ccl::detail::environment::create_device(native_device_type& native_device) const; \
    template ccl::device CCL_API ccl::detail::environment::create_device(const native_device_type& native_device) const;
