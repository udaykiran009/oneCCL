#pragma once
#include "ccl_types.hpp"
#include "ccl_coll_attr.hpp"
#include "coll/coll_attributes.hpp"

namespace ccl
{

template<allgatherv_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>::return_type allgatherv_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type allgatherv_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <allgatherv_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>::return_type& allgatherv_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& allgatherv_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * Allreduce attributes definition
 */
template<allreduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type allreduce_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type allreduce_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <allreduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type& allreduce_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& allreduce_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * alltoall attributes definition
 */
template<alltoall_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>::return_type alltoall_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type alltoall_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <alltoall_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>::return_type& alltoall_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& alltoall_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * alltoallv attributes definition
 */
template<alltoallv_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>::return_type alltoallv_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type alltoallv_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <alltoallv_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>::return_type& alltoallv_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& alltoallv_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * bcast attributes definition
 */
template<bcast_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>::return_type bcast_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type bcast_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <bcast_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>::return_type& bcast_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& bcast_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * reduce attributes definition
 */
template<reduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>::return_type reduce_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type reduce_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <reduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>::return_type& reduce_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& reduce_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * reduce_scatter attributes definition
 */
template<reduce_scatter_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>::return_type reduce_scatter_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type reduce_scatter_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <reduce_scatter_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>::return_type& reduce_scatter_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& reduce_scatter_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * sparse_allreduce attributes definition
 */
template<sparse_allreduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>::return_type sparse_allreduce_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type sparse_allreduce_attr_t::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <sparse_allreduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>::return_type& sparse_allreduce_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& sparse_allreduce_attr_t::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * barrier attributes definition
 */
template<barrier_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>::return_type barrier_attr_t::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type barrier_attr_t::set(const Value& v)
{
    return get_impl().get()->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <barrier_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>::return_type& barrier_attr_t::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& barrier_attr_t::get() const
{
    return get_impl().get()->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}
}
