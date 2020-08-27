#pragma once

#include <memory>
#include <ostream>
#include <utility>
#include <vector>

#include "ccl_types_policy.hpp"
#include "ccl_types.hpp"
#include "ccl_device_types.hpp"
#include "ccl_type_traits.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

#include "ccl_stream.hpp"
#include "ccl_communicator.hpp"

class ccl_comm;
class ccl_event;


namespace ccl {

class stream;
class communicator;
struct communicator_interface;
class kvs_interface;
class request;
class event;

#ifdef DEVICE_COMM_SUPPORT
class device_comm_group;
class device_communicator;
#endif /* DEVICE_COMM_SUPPORT */

/**
 * Types which allow to operate with kvs/communicator/request/stream/event objects in RAII manner
 */
using kvs_t = unique_ptr_class<kvs_interface>;
using communicator_t = unique_ptr_class<communicator>;
using request_t = unique_ptr_class<request>;
using event_t = unique_ptr_class<event>;

#ifdef DEVICE_COMM_SUPPORT
using device_communicator_t = unique_ptr_class<device_communicator>;
using stream_t = unique_ptr_class<stream>;
#endif /* DEVICE_COMM_SUPPORT */
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
    version_t get_version() const;

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

    /**
     * Creates @attr which used to split host communicator
     */
    comm_split_attr_t create_comm_split_attr() const;

#ifdef DEVICE_COMM_SUPPORT

    /**
     * Creates a new device communicators with user supplied size, device indices and kvs.
     * Ranks will be assigned automatically.
     * @param size user-supplied total number of ranks
     * @param devices user-supplied device objects for local ranks
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> create_device_communicators(
        const size_t devices_size,
        const vector_class<native_device_type>& local_devices,
        native_context_type& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates a new device communicators with user supplied size, ranks, device indices and kvs.
     * @param size user-supplied total number of ranks
     * @param rank_device_map user-supplied mapping of local ranks on devices
     * @param context context containing the devices
     * @param kvs key-value store for ranks wire-up
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> create_device_communicators(
        const size_t devices_size,
        vector_class<pair_class<size_t, native_device_type>>& local_rank_device_map,
        native_context_type& context,
        shared_ptr_class<kvs_interface> kvs) const;

    /**
     * Creates @attr which used to split device communicator
     */
    device_comm_split_attr_t create_device_comm_split_attr() const;

    /**
     * Splits device communicators according to attributes.
     * @param attrs split attributes for local communicators
     * @return vector of device communicators
     */
    vector_class<device_communicator_t> split_device_communicators(
        const vector_class<pair_class<device_communicator_t, device_comm_split_attr_t>>& attrs)
        const;

    /**
     * Creates a new stream from @native_stream_type
     * @param native_stream the existing handle of stream
     * @return stream object
     */
    template <class native_stream_type,
              class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
    stream_t create_stream(native_stream_type& native_stream) const;

    /**
     * Creates a new event from @native_event_type
     * @param native_event the existing handle of event
     * @return event object
     */
    template <class native_event_type,
              class = typename std::enable_if<is_event_supported<native_event_type>()>::type>
    event_t create_event(native_event_type& native_event) const;

    // /**
    //  * Creates a new native_stream from @native_stream_type
    //  * @param native_stream the existing handle of stream
    //  * @props are optional specific properties
    //  */
    // template <class native_stream_type,
    //           class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type,
    //           info::stream_property_id... prop_ids>
    // stream_t create_stream(info::arg<prop_ids, info::stream_property_value_t>... props) const {
    //     using tuple_t = tuple_class<info::stream_property_value_t>... > ;
    //     tuple_t args{ props... };

    //     info::stream_properties_data_t stream_args;
    //     arg_filler exec(args);
    //     ccl_tuple_for_each(args, arg_filler);
    //     return create_event(native_stream, stream_args);
    // }

    // /* Example:
    //  *  auto stream = ccl::environment::instance().create_stream(stream_arg(info::stream_atts::ordinal, 0),
    //  *                                                           stream_arg(info::stream_property_id::index, 1),
    //  *                                                           stream_arg(info::stream_property_id::mode, ZE_ASYNC));
    //  *
    //  */
    // stream_t create_stream() const;

#endif /* DEVICE_COMM_SUPPORT */

private:
    // stream_t create_stream(
    //     const info::stream_properties_data_t& args = info::stream_properties_data_t{});

    environment();
};

} // namespace ccl

#ifdef DEVICE_COMM_SUPPORT
#include "ccl_gpu_modules.h"
#include "gpu_communicator.hpp"
#endif /* DEVICE_COMM_SUPPORT */
