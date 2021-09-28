#pragma once

#include "coll/coll_common_attributes.hpp"
#include "common/stream/stream_provider_dispatcher.hpp"
#include "common/utils/utils.hpp"
#include "oneapi/ccl/stream_attr_ids.hpp"
#include "oneapi/ccl/stream_attr_ids_traits.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl/backend_types.hpp>
#endif // CCL_ENABLE_SYCL

namespace ccl {

enum class device_family { unknown, family1, family2 };

std::string to_string(device_family family);

} // namespace ccl

std::string to_string(const stream_type& type);

class alignas(CACHELINE_SIZE) ccl_stream : public stream_provider_dispatcher {
public:
    friend class stream_provider_dispatcher;

    using stream_native_t = stream_provider_dispatcher::stream_native_t;

    ccl_stream() = delete;
    ccl_stream(const ccl_stream& other) = delete;
    ccl_stream& operator=(const ccl_stream& other) = delete;

    ~ccl_stream() = default;

    std::string to_string() const;

    stream_type get_type() const;
    ccl::device_family get_device_family() const;
    bool is_sycl_device_stream() const;
    bool is_gpu() const;

#ifdef CCL_ENABLE_SYCL
    cl::sycl::backend get_backend() const;
#ifdef CCL_ENABLE_ZE
    ze_device_handle_t get_ze_device() const;
    ze_context_handle_t get_ze_context() const;
#endif // CCL_ENABLE_ZE
#endif // CCL_ENBALE_SYCL

    static std::unique_ptr<ccl_stream> create(stream_native_t& native_stream,
                                              const ccl::library_version& version);

    // export attributes
    using version_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::stream_attr_id, ccl::stream_attr_id::version>;
    typename version_traits_t::return_type set_attribute_value(typename version_traits_t::type val,
                                                               const version_traits_t& t);

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const;

    using native_handle_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::stream_attr_id,
                                              ccl::stream_attr_id::native_handle>;
    typename native_handle_traits_t::return_type& get_attribute_value(
        const native_handle_traits_t& id);

private:
    ccl_stream(stream_type type,
               stream_native_t& native_stream,
               const ccl::library_version& version);

    const ccl::library_version version;

    stream_type type;
    ccl::device_family device_family;

#ifdef CCL_ENABLE_SYCL
    cl::sycl::backend backend;

#ifdef CCL_ENABLE_ZE
    ze_device_handle_t device{};
    ze_context_handle_t context{};
#endif // CCL_ENABLE_ZE
#endif // CCL_ENBALE_SYCL
};
