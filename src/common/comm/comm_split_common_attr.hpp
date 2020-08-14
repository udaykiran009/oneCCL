#pragma once
#include "ccl_types.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"

namespace ccl {

/**
 * Base implementation
 */
template< template<class attr, attr id> class traits_t, class split_attrs_t >
class ccl_base_comm_split_attr_impl
{
public:
    /**
     * `version` operations
     */
    using version_traits_t = traits_t<split_attrs_t, split_attrs_t::version>;

    const typename version_traits_t::type&
        get_attribute_value(const version_traits_t& id) const
    {
        return library_version;
    }

    typename version_traits_t::type
        set_attribute_value(typename version_traits_t::type val, const version_traits_t& t)
    {
        (void)t;
        throw ccl_error("Set value for 'version' attribute is not allowed");
        return library_version;
    }

    /**
     * `color` operations
     */
    using color_traits_t = traits_t<split_attrs_t, split_attrs_t::color>;

    const typename color_traits_t::type&
        get_attribute_value(const color_traits_t& id) const
    {
        if (!is_valid<split_attrs_t::color>()) {
            throw ccl_error("Trying to get the value of the attribute 'color' which was not set");
        }
        return color;
    }

    typename color_traits_t::type
        set_attribute_value(typename color_traits_t::type val, const color_traits_t& t)
    {
        auto old = color;
        std::swap(color, val);
        cur_attr = { true, split_attrs_t::color };
        return old;
    }

    /**
     * `group` operations
     */
    using group_traits_t = traits_t<split_attrs_t, split_attrs_t::group>;

    const typename group_traits_t::type&
        get_attribute_value(const group_traits_t& id) const
    {
        if (!is_valid<split_attrs_t::group>()) {
            throw ccl_error("Trying to get the value of the attribute 'group' which was not set");
        }
        return group;
    }

    typename group_traits_t::type
        set_attribute_value(typename group_traits_t::type val, const group_traits_t& t)
    {
        auto old = group;
        std::swap(group, val);
        cur_attr = { true, split_attrs_t::group };
        return old;
    }

    /**
     * Since we can get values for various attributes,
     * we need to have a way to ensure that the requested value has been set.
     * Because if not, an exception is thrown when trying to get a value that was not set.
     * This method helps with it
     */
    template <split_attrs_t attr_id>
    bool is_valid() const noexcept
    {
        return (cur_attr.first && attr_id == cur_attr.second) ||
                                (attr_id == split_attrs_t::version);
    }

    /**
     * Since we can split types: color or group,
     * we need a way to know which specific type we are using.
     * Returns the pair <exist or not; value>
     */
    const std::pair<bool, split_attrs_t>& get_current_split_attr() const noexcept
    {
        return cur_attr;
    }

    constexpr typename color_traits_t::type get_default_color() const
    {
        return 0;
    }

    ccl_base_comm_split_attr_impl(const typename version_traits_t::type& version,
                                    const typename group_traits_t::type& group
                                    ) :
                                        library_version(version),
                                        color(get_default_color()),
                                        group(group),
                                        cur_attr({ false, split_attrs_t::color })
    {
    }

protected:
    const typename version_traits_t::type library_version;
    typename color_traits_t::type color;
    typename group_traits_t::type group;

    template<class T>
    using ccl_optional_t = std::pair<bool /*exist or not*/, T>;

    ccl_optional_t<split_attrs_t> cur_attr;
};





/**
 * Host implementation
 */
class ccl_host_comm_split_attr_impl :
    public ccl_base_comm_split_attr_impl<details::ccl_host_split_traits, ccl_comm_split_attributes>
{
public:
    using base_t = ccl_base_comm_split_attr_impl;

    template <class traits_t>
    const typename traits_t::type& get_attribute_value(const traits_t& id) const
    {
        return base_t::get_attribute_value(id);
    }

    template <class value_t, class traits_t>
    value_t set_attribute_value(value_t val, const traits_t& t)
    {
        return base_t::set_attribute_value(val, t);
    }

    /**
     * Host-specific methods
     */
    constexpr typename group_traits_t::type get_default_group_type() const
    {
        return group_traits_t::type::cluster;    // host-specific value (ccl_group_split_type)
    }

    ccl_host_comm_split_attr_impl(const typename version_traits_t::type& version) :
                                    base_t(
                                        version,
                                        get_default_group_type()
                                    )
    {
    }
};





#ifdef MULTI_GPU_SUPPORT

/**
 * Device implementation
 */
class ccl_device_comm_split_attr_impl :
    public ccl_base_comm_split_attr_impl<details::ccl_device_split_traits, ccl_comm_split_attributes>
{
public:
    using base_t = ccl_base_comm_split_attr_impl;

    template <class traits_t>
    const typename traits_t::type& get_attribute_value(const traits_t& id) const
    {
        return base_t::get_attribute_value(id);
    }

    template <class value_t, class traits_t>
    value_t set_attribute_value(value_t val, const traits_t& t)
    {
        return base_t::set_attribute_value(val, t);
    }

    /**
     * Device-specific methods
     */
    constexpr typename group_traits_t::type get_default_group_type() const
    {
        return group_traits_t::type::cluster;    // device-specific value (ccl_device_group_split_type)
    }

    ccl_device_comm_split_attr_impl(const typename version_traits_t::type& version) :
                                    base_t(
                                        version,
                                        get_default_group_type()
                                    )
    {
    }
};

#endif

} // namespace ccl
