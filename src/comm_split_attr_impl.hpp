#pragma once
#include "ccl_types.hpp"
#include "ccl_comm_split_attr.hpp"

namespace ccl
{

template <class attr_t, class ...attr_value_pair_t>
static attr_t create_attr(attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    auto attr = attr_t(ret);

    int expander [] {(attr.template set<attr_value_pair_t::idx()>(avps.val()), 0)...};
    return attr;
}

template <class ...attr_value_pair_t>
comm_split_attr_t create_comm_split_attr(attr_value_pair_t&&...avps)
{
    return create_attr<comm_split_attr_t>(std::forward<attr_value_pair_t>(avps)...);
}

#ifdef MULTI_GPU_SUPPORT

template <class ...attr_value_pair_t>
device_comm_split_attr_t create_device_comm_split_attr(attr_value_pair_t&&...avps)
{
    return create_attr<device_comm_split_attr_t>(std::forward<attr_value_pair_t>(avps)...);
}

#endif




/**
 * comm_split_attr_t attributes definition
 */
CCL_API comm_split_attr_t::comm_split_attr_t(comm_split_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API comm_split_attr_t::comm_split_attr_t(const comm_split_attr_t& src) :
        base_t(src)
{
}

CCL_API comm_split_attr_t::comm_split_attr_t(const typename details::ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API comm_split_attr_t::~comm_split_attr_t() noexcept
{
}

CCL_API comm_split_attr_t& comm_split_attr_t::operator=(comm_split_attr_t&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}



template<ccl_comm_split_attributes attrId, class Value>
CCL_API Value comm_split_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_host_split_traits<ccl_comm_split_attributes, attrId> {});
}

template <ccl_comm_split_attributes attrId>
CCL_API const typename details::ccl_host_split_traits<ccl_comm_split_attributes, attrId>::type& comm_split_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_host_split_traits<ccl_comm_split_attributes, attrId>{});
}

template <ccl_comm_split_attributes attrId>
CCL_API bool comm_split_attr_t::is_valid() const noexcept
{
    return get_impl()->is_valid<attrId>();
}



#ifdef MULTI_GPU_SUPPORT

/**
 * device_comm_split_attr_t attributes definition
 */
CCL_API device_comm_split_attr_t::device_comm_split_attr_t(device_comm_split_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API device_comm_split_attr_t::device_comm_split_attr_t(const device_comm_split_attr_t& src) :
        base_t(src)
{
}

CCL_API device_comm_split_attr_t::device_comm_split_attr_t(const typename details::ccl_host_split_traits<ccl_comm_split_attributes, ccl_comm_split_attributes::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API device_comm_split_attr_t::~device_comm_split_attr_t() noexcept
{
}

CCL_API device_comm_split_attr_t& device_comm_split_attr_t::operator=(device_comm_split_attr_t&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}



template<ccl_comm_split_attributes attrId, class Value>
CCL_API Value device_comm_split_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_device_split_traits<ccl_comm_split_attributes, attrId> {});
}

template <ccl_comm_split_attributes attrId>
CCL_API const typename details::ccl_device_split_traits<ccl_comm_split_attributes, attrId>::type& device_comm_split_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_device_split_traits<ccl_comm_split_attributes, attrId>{});
}

template <ccl_comm_split_attributes attrId>
CCL_API bool device_comm_split_attr_t::is_valid() const noexcept
{
    return get_impl()->is_valid<attrId>();
}

#endif

} // namespace ccl
