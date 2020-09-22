#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"
#include "coll/coll_attributes.hpp"

namespace ccl
{

template<allgatherv_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<allgatherv_attr_id, attrId>::return_type allgatherv_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allgatherv_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type allgatherv_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <allgatherv_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allgatherv_attr_id, attrId>::return_type& allgatherv_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allgatherv_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& allgatherv_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}

/**
 * Allreduce attributes definition
 */
template<allreduce_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<allreduce_attr_id, attrId>::return_type allreduce_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allreduce_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type allreduce_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <allreduce_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allreduce_attr_id, attrId>::return_type& allreduce_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allreduce_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& allreduce_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}


/**
 * alltoall attributes definition
 */
template<alltoall_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<alltoall_attr_id, attrId>::return_type alltoall_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoall_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type alltoall_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <alltoall_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoall_attr_id, attrId>::return_type& alltoall_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoall_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& alltoall_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}


/**
 * alltoallv attributes definition
 */
template<alltoallv_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<alltoallv_attr_id, attrId>::return_type alltoallv_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoallv_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type alltoallv_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <alltoallv_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoallv_attr_id, attrId>::return_type& alltoallv_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoallv_attr_id, attrId>{});
}

template <operation_attr_id attrId>
const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& alltoallv_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}


/**
 * bcast attributes definition
 */
template<broadcast_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<broadcast_attr_id, attrId>::return_type broadcast_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<broadcast_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type broadcast_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <broadcast_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<broadcast_attr_id, attrId>::return_type& broadcast_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<broadcast_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& broadcast_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}

/**
 * reduce attributes definition
 */
template<reduce_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<reduce_attr_id, attrId>::return_type reduce_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type reduce_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <reduce_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_attr_id, attrId>::return_type& reduce_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& reduce_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}

/**
 * reduce_scatter attributes definition
 */
template<reduce_scatter_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<reduce_scatter_attr_id, attrId>::return_type reduce_scatter_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_scatter_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type reduce_scatter_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <reduce_scatter_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_scatter_attr_id, attrId>::return_type& reduce_scatter_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_scatter_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& reduce_scatter_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}


/**
 * sparse_allreduce attributes definition
 */
template<sparse_allreduce_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, attrId>::return_type sparse_allreduce_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type sparse_allreduce_attr::set(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <sparse_allreduce_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, attrId>::return_type& sparse_allreduce_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<sparse_allreduce_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& sparse_allreduce_attr::get() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}

/**
 * barrier attributes definition
 */
template<barrier_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<barrier_attr_id, attrId>::return_type barrier_attr::set(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<barrier_attr_id, attrId> {});
}

template<operation_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type barrier_attr::set(const Value& v)
{
    return get_impl().get()->set_attribute_value(v, details::ccl_api_type_attr_traits<operation_attr_id, attrId> {});
}

template <barrier_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<barrier_attr_id, attrId>::return_type& barrier_attr::get() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<barrier_attr_id, attrId>{});
}

template <operation_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<operation_attr_id, attrId>::return_type& barrier_attr::get() const
{
    return get_impl().get()->get_attribute_value(details::ccl_api_type_attr_traits<operation_attr_id, attrId>{});
}
}
