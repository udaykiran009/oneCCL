#pragma once

#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/kvs_attr_ids_traits.hpp"

namespace ccl {

class ccl_kvs_attr_impl {
public:
    /**
     * `version` operations
     */
    using version_traits_t = detail::ccl_api_type_attr_traits<kvs_attr_id, kvs_attr_id::version>;
    using ip_port_traits_t = detail::ccl_api_type_attr_traits<kvs_attr_id, kvs_attr_id::ip_port>;

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const {
        return version;
    }

    typename version_traits_t::return_type set_attribute_value(typename version_traits_t::type val,
                                                               const version_traits_t& t) {
        (void)t;
        throw ccl::exception("Set value for 'ccl::kvs_attr_id::version' is not allowed");
        return version;
    }

    const typename ip_port_traits_t::return_type& get_attribute_value(
        const ip_port_traits_t& id) const {
        return ip_port;
    }

    typename ip_port_traits_t::type set_attribute_value(typename ip_port_traits_t::type val,
                                                        const ip_port_traits_t& t) {
        auto old = ip_port;
        std::swap(ip_port, val);
        cur_attr = { true, kvs_attr_id::ip_port };
        return old;
    }

    ccl_kvs_attr_impl(const typename version_traits_t::return_type& version) : version(version) {}

    template <kvs_attr_id attr_id>
    bool is_valid() const noexcept {
        return (cur_attr.first && attr_id == cur_attr.second) || (attr_id == kvs_attr_id::version);
    }

protected:
    typename version_traits_t::return_type version;
    typename ip_port_traits_t::return_type ip_port{};
    template <class T>
    using ccl_optional_t = std::pair<bool /*exist or not*/, T>;

    ccl_optional_t<kvs_attr_id> cur_attr;
};

} // namespace ccl
