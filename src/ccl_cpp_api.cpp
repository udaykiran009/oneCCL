#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

#include "common/comm/comm_interface.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

#define CCL_CHECK_AND_THROW(result, diagnostic)   \
  do {                                            \
      if (result != ccl_status_success)           \
      {                                           \
          throw ccl_error(diagnostic);            \
      }                                           \
  } while (0);


namespace ccl
{

CCL_API ccl::environment::environment()
{
    static auto result = global_data::get().init();
    CCL_CHECK_AND_THROW(result, "failed to initialize CCL");
}

CCL_API ccl::environment::~environment()
{}

CCL_API ccl::environment& ccl::environment::instance()
{
    static ccl::environment env;
    return env;
}

void CCL_API ccl::environment::set_resize_fn(ccl_resize_fn_t callback)
{
    ccl_status_t result = ccl_set_resize_fn(callback);
    CCL_CHECK_AND_THROW(result, "failed to set resize callback");
    return;
}

ccl_version_t CCL_API ccl::environment::get_version() const
{
    ccl_version_t ret;
    ccl_status_t result = ccl_get_version(&ret);
    CCL_CHECK_AND_THROW(result, "failed to get version");
    return ret;
}
/*
static ccl::stream& get_empty_stream()
{
    static ccl::stream_t empty_stream  = ccl::environment::instance().create_stream();
    return empty_stream;
}
*/

/**
 * Factory methods
 */
//Communicator
communicator environment::create_communicator() const
{
    return communicator::create_communicator();
}

communicator environment::create_communicator(const size_t size,
                                       shared_ptr_class<kvs_interface> kvs) const
{
    return communicator::create_communicator(size, kvs);
}

communicator environment::create_communicator(const size_t size,
                                     const size_t rank,
                                     shared_ptr_class<kvs_interface> kvs) const
{
    return communicator::create_communicator(size, rank, kvs);
}

template <class ...attr_value_pair_t>
comm_split_attr_t environment::create_comm_split_attr(attr_value_pair_t&&...avps) const
{
    return comm_split_attr_t::create_comm_split_attr(std::forward<attr_value_pair_t>(avps)...);
}

//Device communicator
#ifdef DEVICE_COMM_SUPPORT

template <class ...attr_value_pair_t>
device_comm_split_attr_t environment::create_device_comm_split_attr(attr_value_pair_t&&...avps) const
{
    return device_comm_split_attr_t::create_device_comm_split_attr(std::forward<attr_value_pair_t>(avps)...);
}

template<class DeviceType,
             class ContextType>
vector_class<device_communicator> environment::create_device_communicators(
        const size_t devices_size,
        const vector_class<DeviceType>& local_devices,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs) const
{
    return device_communicator::create_device_communicators(devices_size, local_devices, context, kvs);
}

template<class DeviceType,
         class ContextType>
vector_class<device_communicator> environment::create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const vector_class<pair_class<rank_t, DeviceType>>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
    return device_communicator::create_device_communicators(cluster_devices_size, local_rank_device_map, context, kvs);
}


template<class DeviceType,
         class ContextType>
vector_class<device_communicator> environment::create_device_communicators(
        const size_t cluster_devices_size, /*global devics count*/
        const map_class<rank_t, DeviceType>& local_rank_device_map,
        ContextType& context,
        shared_ptr_class<kvs_interface> kvs)
{
    return device_communicator::create_device_communicators(cluster_devices_size, local_rank_device_map, context, kvs);
}


//Stream
template <class native_stream_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
stream environment::create_stream(native_stream_type& native_stream)
{
    return stream::create_stream(native_stream);
}

template <class native_stream_type, class native_context_type,
          class = typename std::enable_if<is_stream_supported<native_stream_type>()>::type>
stream environment::create_stream(native_stream_type& native_stream, native_context_type& native_ctx)
{
    return stream::create_stream(native_stream, ctx);
}

template <class ...attr_value_pair_t>
stream environment::create_stream_from_attr(typename unified_device_type::ccl_native_t device, attr_value_pair_t&&...avps)
{
    return stream::create_stream_from_attr(device, std::forward<attr_value_pair_t>(avps)...);
}

template <class ...attr_value_pair_t>
stream environment::create_stream_from_attr(typename unified_device_type::ccl_native_t device,
                               typename unified_device_context_type::ccl_native_t context,
                               attr_value_pair_t&&...avps)
{
    return stream::create_stream_from_attr(device, context, std::forward<attr_value_pair_t>(avps)...);
}


//Event
template <class event_type,
          class = typename std::enable_if<is_event_supported<event_type>()>::type>
event environment::create_event(event_type& native_event)
{
    return event::create_event(native_event);
}

template <class event_type,
          class ...attr_value_pair_t>
event environment::create_event_from_attr(event_type& native_event_handle,
                             typename unified_device_context_type::ccl_native_t context,
                             attr_value_pair_t&&...avps)
{
    return event::create_event_from_attr(native_event, context,  std::forward<attr_value_pair_t>(avps)...);
}
/*
#define STREAM_CREATOR_INSTANTIATION(type)                                                                                                           \
template ccl::stream_t CCL_API ccl::environment::create_stream(type& stream);

#ifdef CCL_ENABLE_SYCL
STREAM_CREATOR_INSTANTIATION(cl::sycl::queue)
#endif
*/
#endif //DEVICE_COMM_SUPPORT
#if 0
ccl::datatype CCL_API ccl::datatype_create(const ccl::datatype_attr_t* attr)
{
    ccl_datatype_t dtype;
    ccl_datatype_create(&dtype, attr);
    return static_cast<ccl::datatype>(dtype);
}

void CCL_API ccl::datatype_free(ccl::datatype dtype)
{
    ccl_datatype_free(dtype);
}

size_t CCL_API ccl::datatype_get_size(ccl::datatype dtype)
{
    size_t size;
    ccl_get_datatype_size(dtype, &size);
    return size;
}
#endif
}
#include "types_generator_defines.hpp"
#include "ccl_cpp_api_explicit_in.hpp"
