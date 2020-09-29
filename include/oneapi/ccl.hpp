#pragma once

#include "oneapi/ccl/ccl_environment.hpp"
#include "oneapi/ccl/ccl_api_functions.hpp"

#ifdef MULTI_GPU_SUPPORT
#include "oneapi/ccl/ccl_gpu_modules.h"
#endif /* MULTI_GPU_SUPPORT */

namespace ccl {}
namespace oneapi {
namespace ccl = ::ccl;
}

#if 0
#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "oneapi/ccl/ccl_types_policy.hpp"
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

#include "common/event/event_internal/event_internal_attr_ids.hpp"
#include "common/event/event_internal/event_internal_attr_ids_traits.hpp"
#include "common/event/event_internal/event_internal.hpp"

#include "oneapi/ccl/ccl_kvs.hpp"

#include "oneapi/ccl/ccl_event.hpp"

#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "oneapi/ccl/ccl_communicator.hpp"

namespace ccl {
/**
 * Types which allow to operate with kvs/communicator/request/stream/event objects in RAII manner
 */
using kvs_t = shared_ptr_class<kvs_interface>;

/**
 * CCL environment singleton
 */
class environment {
public:
    ~environment();

    /**
     * Retrieves the unique environment object
     * and makes the first-time initialization of CCL library
     */
    static environment& instance();

    /**
     * Retrieves the current version
     */
    ccl::library_version get_library_version() const;
#if 0
    /**
     * Creates @attr which used to register custom datatype
     */
    datatype_attr_t create_datatype_attr() const;

    /**
     * Registers custom datatype to be used in communication operations
     * @param attr datatype attributes
     * @return datatype handle
     */
    datatype register_datatype(const datatype_attr_t& attr);

    /**
     * Deregisters custom datatype
     * @param dtype custom datatype handle
     */
    void deregister_datatype(datatype dtype);
#endif
    void set_resize_fn(ccl_resize_fn_t callback);

    /**
     * Retrieves a datatype size in bytes
     * @param dtype datatype handle
     * @return datatype size
     */
    size_t get_datatype_size(datatype dtype) const;

    /**
     * Creates a main key-value store.
     * It's address should be distributed using out of band communication mechanism
     * and be used to create key-value stores on other ranks.
     * @return kvs object
     */
    kvs_t create_main_kvs() const;

    /**
     * Creates a new key-value store from main kvs address
     * @param addr address of main kvs
     * @return kvs object
     */
    kvs_t create_kvs(const kvs::addr_t& addr) const;

    template <class ...attr_value_pair_t>
    comm_split_attr create_device_comm_split_attr(attr_value_pair_t&&...avps) const;

#ifdef MULTI_GPU_SUPPORT

    /**
     * Creates a new device communicators with user supplied size, device indices and kvs.
     * Ranks will be assigned automatically.
     * @param size user-supplied total number of ranks
     * @param devices user-supplied device objects for local ranks
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    template<class DeviceType,
             class ContextType>
    vector_class<communicator> create_device_communicators(
        const size_t devices_size,
        const vector_class<DeviceType>& local_devices,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates a new device communicators with user supplied size, ranks, device indices and kvs.
     * @param size user-supplied total number of ranks
     * @param rank_device_map user-supplied mapping of local ranks on devices
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    template<class DeviceType,
         class ContextType>
    vector_class<communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs);


    template<class DeviceType,
         class ContextType>
    vector_class<communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const map_class<rank_t, DeviceType>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs);

    /**
     * Splits device communicators according to attributes.
     * @param attrs split attributes for local communicators
     * @return vector of device communicators
     */
    vector_class<communicator> split_device_communicators(
        const vector_class<pair_class<communicator, comm_split_attr>>& attrs)
        const;

    /**
     * Creates a new stream from @native_stream_type
     * @param native_stream the existing handle of stream
     * @return stream object
     */
    template <class native_stream_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    stream create_stream(native_stream_type& native_stream);

    template <class native_stream_type, class native_context_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    stream create_stream(native_stream_type& native_stream, native_context_type& native_ctx);

    template <class ...attr_value_pair_t>
    stream create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps);

    template <class ...attr_value_pair_t>
    stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps);

    /**
     * Creates a new event from @native_event_type
     * @param native_event the existing handle of event
     * @return event object
     */
    template <class event_type,
          class = typename std::enable_if<is_event_supported<event_type>()>::type>
    event create_event(event_type& native_event);

    template <class event_type,
          class ...attr_value_pair_t>
    event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps);

#endif /* MULTI_GPU_SUPPORT */

private:
    environment();
};

} // namespace ccl

#ifdef MULTI_GPU_SUPPORT
#include "oneapi/ccl/ccl_gpu_modules.h"
#endif /* MULTI_GPU_SUPPORT */

#endif
