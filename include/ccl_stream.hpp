#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

class ccl_stream;
namespace ccl {

/**
 * A stream object is an abstraction over CPU/GPU streams
 * Has no defined public constructor. Use ccl::environment::create_stream
 * for stream objects creation
 */
namespace info {
/**
 * Stream attribute ids
 */
enum class stream_attr_id : int {
    device,
    properties,

    last_value
};

/**
 * Stream `properties` attributes flags
 */
enum class stream_properties : int {
    ordinal,
    index,
    flags,
    mode,
    priority,

    last_value
};
/**
 *Stream `properties` attributes flag values
 */
using stream_property_value_t = uint32_t;
using stream_properties_data_t = std::array<stream_property_value_t, static_cast<typename std::underlying_type<stream_properties>::type>(stream_properties::last_value)>;

template <stream_properties index, class value_t>
struct arg {
    using value_type = value_t;
    arg(value_t val) : m_val(val) {}

    static constexpr stream_properties idx() {
        return index;
    }
    const value_type val() {
        return m_val;
    }
    value_t m_val;
};

template <stream_properties ids>
constexpr arg<ids, stream_property_value_t> stream_args(stream_property_value_t val) {
    return arg<ids, stream_property_value_t>(val);
}

/**
 * Traits specialization for stream attributes
 */
template <>
struct param_traits<stream_attr_id, stream_attr_id::device> {
    using type = typename ccl::unified_device_type::native_reference_t;
};

template <>
struct param_traits<stream_attr_id, stream_attr_id::properties> {
    using type = stream_properties_data_t;
};
} // namespace info

/**
 * Stream class
 */
class stream : public non_copyable<stream>,
               public non_movable<stream>,
               public pointer_on_impl<stream, ccl_stream> {
public:
    using native_handle_t = typename ccl::unified_stream_type::native_reference_t;
    using impl_value_t = typename pointer_on_impl<stream, ccl_stream>::impl_value_t;

    using creation_args_t = native_handle_t;
    using optional_args_t =
        typename info::param_traits<info::stream_attr_id, info::stream_attr_id::properties>::type;

    native_handle_t get() const;

    template <info::stream_attr_id param>
    typename info::param_traits<info::stream_attr_id, param>::type get_info() const;

private:
    friend class communicator;
    friend class environment;

    stream(impl_value_t&& impl);
};
} // namespace ccl
