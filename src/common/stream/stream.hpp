#pragma once
#include "ccl_types_policy.hpp"
#include "ccl_types.hpp"
#include "ccl_type_traits.hpp"
#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "common/utils/utils.hpp"
#include "common/stream/stream_provider_dispatcher.hpp"

namespace ccl
{
    class environment;  //friend-zone
}

ccl_status_t CCL_API ccl_stream_create(ccl_stream_type_t type,
                                          void* native_stream,
                                          ccl_stream_t* stream);

class alignas(CACHELINE_SIZE) ccl_stream : public stream_provider_dispatcher
{
public:
    friend class stream_provider_dispatcher;
    friend class ccl::environment;
    friend ccl_status_t CCL_API ccl_stream_create(ccl_stream_type_t type,
                               void* native_stream,
                               ccl_stream_t* stream);
    using stream_native_t = stream_provider_dispatcher::stream_native_t;
    using stream_native_handle_t = stream_provider_dispatcher::stream_native_handle_t;

    ccl_stream() = delete;
    ccl_stream(const ccl_stream& other) = delete;
    ccl_stream& operator=(const ccl_stream& other) = delete;

    ~ccl_stream() = default;

    using stream_provider_dispatcher::get_native_stream;
    using stream_provider_dispatcher::get_native_stream_handle;

    ccl_stream_type_t get_type() const
    {
        return type;
    }
    bool is_sycl_device_stream() const
    {
        return (type == ccl_stream_cpu || type == ccl_stream_gpu);
    }

    static std::unique_ptr<ccl_stream> create(stream_native_t& native_stream, const ccl_version_t& version);

    //Export Attributes
    using version_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::version>;
    typename version_traits_t::type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t);

    const typename version_traits_t::return_type&
        get_attribute_value(const version_traits_t& id) const;


    using native_handle_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::native_handle>;
    typename native_handle_traits_t::return_type
        get_attribute_value(const native_handle_traits_t& id);


    using device_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::device>;
    typename device_traits_t::return_type
        get_attribute_value(const device_traits_t& id);


    using context_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::context>;
    typename context_traits_t::return_type
        get_attribute_value(const context_traits_t& id);


    using ordinal_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::ordinal>;
    typename ordinal_traits_t::return_type
        set_attribute_value(typename ordinal_traits_t::type val, const ordinal_traits_t& t);

    const typename ordinal_traits_t::return_type&
        get_attribute_value(const ordinal_traits_t& id) const;


    using index_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::index>;
    typename index_traits_t::return_type
        set_attribute_value(typename index_traits_t::type val, const index_traits_t& t);

    const typename index_traits_t::return_type&
        get_attribute_value(const index_traits_t& id) const;


    using flags_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::flags>;
    typename flags_traits_t::return_type
        set_attribute_value(typename flags_traits_t::type val, const flags_traits_t& t);

    const typename flags_traits_t::return_type&
        get_attribute_value(const flags_traits_t& id) const;


    using mode_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::mode>;
    typename mode_traits_t::return_type
        set_attribute_value(typename mode_traits_t::type val, const mode_traits_t& t);

    const typename mode_traits_t::return_type&
        get_attribute_value(const mode_traits_t& id) const;


    using priority_traits_t = ccl::details::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::priority>;
    typename priority_traits_t::return_type
        set_attribute_value(typename priority_traits_t::type val, const priority_traits_t& t);

    const typename priority_traits_t::return_type&
        get_attribute_value(const priority_traits_t& id) const;

    void build_from_attr();
private:
    template<class NativeStream,
             typename std::enable_if<std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                     int>::type = 0>
    ccl_stream(ccl_stream_type_t stream_type, NativeStream& native_stream, const ccl_version_t& version) :
        stream_provider_dispatcher(native_stream),
        type(stream_type),
        library_version(version)
    {
    }
    template<class NativeStream,
             typename std::enable_if<not std::is_class<typename std::remove_cv<NativeStream>::type>::value,
                                     int>::type = 0>
    ccl_stream(ccl_stream_type_t stream_type, NativeStream native_stream, const ccl_version_t& version) :
        stream_provider_dispatcher(native_stream),
        type(stream_type),
        library_version(version)
    {
    }

    ccl_stream_type_t type;
    const ccl_version_t library_version;
    typename ordinal_traits_t::return_type ordinal_val;
    typename index_traits_t::return_type index_val;
    typename flags_traits_t::return_type flags_val;
    typename mode_traits_t::return_type mode_val;
    typename priority_traits_t::return_type priority_val;
};
