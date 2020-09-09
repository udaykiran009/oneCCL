#pragma once

#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "ccl_types_policy.hpp"
#include "ccl_types.hpp"
#include "ccl_type_traits.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

#include "ccl_datatype_attr_ids.hpp"
#include "ccl_datatype_attr_ids_traits.hpp"
#include "ccl_datatype_attr.hpp"

#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_kvs.hpp"

#include "ccl_request.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

#include "ccl_communicator.hpp"
#include "ccl_device_communicator.hpp"

namespace ccl {

/**
 * Helper functions to create custom datatype.
 */
datatype datatype_create(const datatype_attr_t* attr);
void datatype_free(datatype dtype);
size_t datatype_get_size(datatype dtype);


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
    ccl_version_t get_version() const;

    /**
     * Creates @attr which used to register custom datatype
     */
    template <class ...attr_value_pair_t>
    datatype_attr_t create_datatype_attr(attr_value_pair_t&&...avps) const
    {
        static_assert(sizeof...(avps) > 0, "At least one argument must be specified");
        auto datatype_attr = create_postponed_api_type<datatype_attr_t>();
        int expander [] {(datatype_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void)expander;
        return datatype_attr;
    }

    /**
     * Registers custom datatype to be used in communication operations
     * @param attr datatype attributes
     * @return datatype handle
     */
    ccl_datatype_t register_datatype(const ccl::datatype_attr_t& attr);

    /**
     * Deregisters custom datatype
     * @param dtype custom datatype handle
     */
    void deregister_datatype(ccl_datatype_t dtype);

    size_t get_datatype_size(ccl_datatype_t dtype) const;

    /**
     * Enables job scalability policy
     * @param callback of @c ccl_resize_fn_t type, which enables scalability policy
     * (@c nullptr enables default behavior)
     */
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
    shared_ptr_class<kvs> create_main_kvs() const;

    /**
     * Creates a new key-value store from main kvs address
     * @param addr address of main kvs
     * @return kvs object
     */
    shared_ptr_class<kvs> create_kvs(const kvs::addr_t& addr) const;

    /**
     * Creates a new host communicator with externally provided size, rank and kvs.
     * Implementation is platform specific and non portable.
     * @return host communicator
     */
    communicator create_communicator() const;

    /**
     * Creates a new host communicator with user supplied size and kvs.
     * Rank will be assigned automatically.
     * @param size user-supplied total number of ranks
     * @param kvs key-value store for ranks wire-up
     * @return host communicator
     */
    communicator create_communicator(const size_t size,
                                       shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates a new host communicator with user supplied size, rank and kvs.
     * @param size user-supplied total number of ranks
     * @param rank user-supplied rank
     * @param kvs key-value store for ranks wire-up
     * @return host communicator
     */
    communicator create_communicator(const size_t size,
                                     const size_t rank,
                                     shared_ptr_class<kvs_interface> kvs) const;

    template <class coll_attribute_type,
              class ...attr_value_pair_t>
    coll_attribute_type create_op_attr(attr_value_pair_t&&...avps) const
    {
        auto coll_attr = create_postponed_api_type<coll_attribute_type>();
        int expander [] {(coll_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void)expander;
        return coll_attr;
    }

    /**
     * Creates @attr which used to split host communicator
     */
    template <class ...attr_value_pair_t>
    comm_split_attr_t create_comm_split_attr(attr_value_pair_t&&...avps) const
    {
        auto split_attr = create_postponed_api_type<comm_split_attr_t>();
        int expander [] {(split_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void)expander;
        return split_attr;
    }

#ifdef CCL_ENABLE_SYCL
    device_communicator create_single_device_communicator(const size_t world_size,
                                     const size_t rank,
                                     const cl::sycl::device &device,
                                     shared_ptr_class<kvs_interface> kvs) const;

    device_communicator create_single_device_communicator(const size_t world_size,
                                     const size_t rank,
                                     cl::sycl::queue queue,
                                     shared_ptr_class<kvs_interface> kvs) const;

    template<class DeviceSelectorType>
    device_communicator create_single_device_communicator(const size_t world_size,
                                     const size_t rank,
                                     const DeviceSelectorType& selector,
                                     shared_ptr_class<kvs_interface> kvs) const
    {
        return create_single_device_communicator(world_size, rank, cl::sycl::device(selector), kvs);
    }


#endif
#ifdef MULTI_GPU_SUPPORT

    template <class ...attr_value_pair_t>
    device_comm_split_attr_t create_device_comm_split_attr(attr_value_pair_t&&...avps) const
    {
        auto split_attr = create_postponed_api_type<device_comm_split_attr_t>();
        int expander [] {(split_attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void)expander;
        return split_attr;
    }

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
    vector_class<device_communicator> create_device_communicators(
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
    vector_class<device_communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs) const;


    template<class DeviceType,
         class ContextType>
    vector_class<device_communicator> create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const map_class<rank_t, DeviceType>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Splits device communicators according to attributes.
     * @param attrs split attributes for local communicators
     * @return vector of device communicators
     */
    vector_class<device_communicator> split_device_communicators(
        const vector_class<pair_class<device_communicator, device_comm_split_attr_t>>& attrs)
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
    stream create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps)
    {
        stream str = create_postponed_api_type<stream>(device);
        int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void) expander;
        str.build_from_params();
        return str;
    }

    template <class ...attr_value_pair_t>
    stream create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps)
    {
        stream str = create_postponed_api_type<stream>(device, context);
        int expander [] {(str.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void) expander;
        str.build_from_params();
        return str;
    }

    /**
     * Creates a new event from @native_event_type
     * @param native_event the existing handle of event
     * @return event object
     */
    template <class event_type,
          class = typename std::enable_if<is_event_supported<event_type>()>::type>
    event create_event(event_type& native_event);

    template <class event_handle_type,
              class = typename std::enable_if<is_event_supported<event_handle_type>()>::type>
    event create_event(event_handle_type native_event_handle, typename unified_device_context_type::ccl_native_t context);

    template <class event_type,
          class ...attr_value_pair_t>
    event create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps)
    {
        event ev = create_postponed_api_type<event>(native_event_handle, context);
        int expander [] {(ev.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
        (void) expander;
        ev.build_from_params();
        return ev;
    }


#endif /* MULTI_GPU_SUPPORT */

private:
    environment();

    template<class ccl_api_type, class ...args_type>
    ccl_api_type create_postponed_api_type(args_type... args) const;
};

} // namespace ccl
