#include "ccl_types.h"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

// Core file with PIMPL implementation
#include "common/comm/comm_split_common_attr.hpp"
#include "comm_split_attr_impl.hpp"

namespace ccl
{
#define COMMA   ,
#define API_FORCE_INSTANTIATION(class_name, IN_attrId, IN_Value, OUT_Traits_Value)                  \
template                                                                                            \
CCL_API IN_Value class_name::set<IN_attrId, IN_Value>(const IN_Value& v);                           \
                                                                                                    \
template                                                                                            \
CCL_API const typename details::OUT_Traits_Value<ccl_comm_split_attributes, IN_attrId>::type& class_name::get<IN_attrId>() const; \
                                                                                                    \
template                                                                                            \
CCL_API bool class_name::is_valid<IN_attrId>() const noexcept;



/**
 * comm_split_attr_t attributes definition
 */
CCL_API comm_split_attr_t::comm_split_attr_t(ccl_empty_attr) :
        base_t(std::shared_ptr<impl_t>(new impl_t(ccl_empty_attr::version)))
{

}
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
API_FORCE_INSTANTIATION(comm_split_attr_t, ccl_comm_split_attributes::color, int, ccl_host_split_traits)
API_FORCE_INSTANTIATION(comm_split_attr_t, ccl_comm_split_attributes::group, ccl_group_split_type, ccl_host_split_traits)
API_FORCE_INSTANTIATION(comm_split_attr_t, ccl_comm_split_attributes::version, ccl_version_t, ccl_host_split_traits)




#ifdef MULTI_GPU_SUPPORT

/**
 * device_comm_split_attr_t attributes definition
 */
CCL_API device_comm_split_attr_t::device_comm_split_attr_t(ccl_empty_attr) :
        base_t(std::shared_ptr<impl_t>(new impl_t(ccl_empty_attr::version)))
{

}
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
API_FORCE_INSTANTIATION(device_comm_split_attr_t, ccl_comm_split_attributes::color, int, ccl_device_split_traits)
API_FORCE_INSTANTIATION(device_comm_split_attr_t, ccl_comm_split_attributes::group, device_group_split_type, ccl_device_split_traits)
API_FORCE_INSTANTIATION(device_comm_split_attr_t, ccl_comm_split_attributes::version, ccl_version_t, ccl_device_split_traits)

#endif

#undef API_FORCE_INSTANTIATION
#undef COMMA
}
