#pragma once
#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "common/event/ccl_event_attr_ids.hpp"
#include "common/event/ccl_event_attr_ids_traits.hpp"
#include "common/utils/utils.hpp"

namespace ccl {
namespace detail {
class environment;
}
} // namespace ccl

class alignas(CACHELINE_SIZE) ccl_event {
public:
    friend class ccl::detail::environment;
    using event_native_handle_t = typename ccl::unified_event_type::handle_t;
    using event_native_t = typename ccl::unified_event_type::ccl_native_t;

    ccl_event() = delete;
    ccl_event(const ccl_event& other) = delete;
    ccl_event& operator=(const ccl_event& other) = delete;

    ccl_event(event_native_t& event, const ccl::library_version& version);
    ~ccl_event() = default;

    //Export Attributes
    using version_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::event_attr_id, ccl::event_attr_id::version>;
    typename version_traits_t::type set_attribute_value(typename version_traits_t::type val,
                                                        const version_traits_t& t);

    const typename version_traits_t::return_type& get_attribute_value(
        const version_traits_t& id) const;

    using native_handle_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::event_attr_id,
                                              ccl::event_attr_id::native_handle>;
    typename native_handle_traits_t::return_type& get_attribute_value(
        const native_handle_traits_t& id);

    using command_type_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::event_attr_id, ccl::event_attr_id::command_type>;
    typename command_type_traits_t::return_type set_attribute_value(
        typename command_type_traits_t::type val,
        const command_type_traits_t& t);

    const typename command_type_traits_t::return_type& get_attribute_value(
        const command_type_traits_t& id) const;

    using command_execution_status_traits_t =
        ccl::detail::ccl_api_type_attr_traits<ccl::event_attr_id,
                                              ccl::event_attr_id::command_execution_status>;
    typename command_execution_status_traits_t::return_type set_attribute_value(
        typename command_execution_status_traits_t::type val,
        const command_execution_status_traits_t& t);

    const typename command_execution_status_traits_t::return_type& get_attribute_value(
        const command_execution_status_traits_t& id) const;

    void build_from_params();

private:
    const ccl::library_version version;
    event_native_t native_event;
    bool creation_is_postponed{ false };

    typename command_type_traits_t::return_type command_type_val;
    typename command_execution_status_traits_t::return_type command_execution_status_val;
};
