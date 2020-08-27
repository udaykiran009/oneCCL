#include "ccl_types.h"
#include "ccl_aliases.hpp"
#include "ccl_types_policy.hpp"
#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

// Core file with PIMPL implementation
#include "coll_attr_impl.hpp"
#include "coll/coll_attributes.hpp"

namespace ccl
{
#define COMMA   ,

#define API_FORCE_INSTANTIATION_SET(class_name, IN_attrType, IN_attrId, IN_Value)                        \
template                                                                                            \
CCL_API typename details::ccl_api_type_attr_traits<IN_attrType, IN_attrId>::return_type class_name::set<IN_attrId, IN_Value>(const IN_Value& v);

#define API_FORCE_INSTANTIATION_GET(class_name, IN_attrType, IN_attrId)                        \
template                                                                                            \
CCL_API const typename details::ccl_api_type_attr_traits<IN_attrType, IN_attrId>::return_type& class_name::get<IN_attrId>() const;


#define API_FORCE_INSTANTIATION(class_name, IN_attrType, IN_attrId, IN_Value)                       \
API_FORCE_INSTANTIATION_SET(class_name, IN_attrType, IN_attrId, IN_Value)                           \
API_FORCE_INSTANTIATION_GET(class_name, IN_attrType, IN_attrId)                                     \




#define COMMON_API_FORCE_INSTANTIATION(class_name)                                                  \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::version, ccl_version_t)    \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::prolog_fn, ccl_prologue_fn_t) \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::epilog_fn, ccl_epilogue_fn_t)  \
                                                                                                    \
API_FORCE_INSTANTIATION_SET(class_name, common_op_attr_id, common_op_attr_id::priority, size_t)  \
API_FORCE_INSTANTIATION_SET(class_name, common_op_attr_id, common_op_attr_id::priority, int)  \
API_FORCE_INSTANTIATION_SET(class_name, common_op_attr_id, common_op_attr_id::priority, unsigned int)  \
API_FORCE_INSTANTIATION_GET(class_name, common_op_attr_id, common_op_attr_id::priority)             \
                                                                                                    \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::synchronous, int)  \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::to_cache, int) \
API_FORCE_INSTANTIATION(class_name, common_op_attr_id, common_op_attr_id::match_id, ccl::string_class)


/**
 * allgatherv coll attributes
 */
CCL_API allgatherv_attr_t::allgatherv_attr_t(allgatherv_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API allgatherv_attr_t::allgatherv_attr_t(const allgatherv_attr_t& src) :
        base_t(src)
{
}

CCL_API allgatherv_attr_t::allgatherv_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API allgatherv_attr_t::~allgatherv_attr_t()
{
}



/**
 * allreduce coll attributes
 */
CCL_API allreduce_attr_t::allreduce_attr_t(allreduce_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API allreduce_attr_t::allreduce_attr_t(const allreduce_attr_t& src) :
        base_t(src)
{
}

CCL_API allreduce_attr_t::allreduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API allreduce_attr_t::~allreduce_attr_t()
{
}


/**
 * alltoall coll attributes
 */
CCL_API alltoall_attr_t::alltoall_attr_t(alltoall_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API alltoall_attr_t::alltoall_attr_t(const alltoall_attr_t& src) :
        base_t(src)
{
}

CCL_API alltoall_attr_t::alltoall_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API alltoall_attr_t::~alltoall_attr_t()
{
}


/**
 * alltoallv coll attributes
 */
CCL_API alltoallv_attr_t::alltoallv_attr_t(alltoallv_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API alltoallv_attr_t::alltoallv_attr_t(const alltoallv_attr_t& src) :
        base_t(src)
{
}

CCL_API alltoallv_attr_t::alltoallv_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API alltoallv_attr_t::~alltoallv_attr_t()
{
}


/**
 * bcast coll attributes
 */
CCL_API bcast_attr_t::bcast_attr_t(bcast_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API bcast_attr_t::bcast_attr_t(const bcast_attr_t& src) :
        base_t(src)
{
}

CCL_API bcast_attr_t::bcast_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API bcast_attr_t::~bcast_attr_t()
{
}


/**
 * reduce coll attributes
 */
CCL_API reduce_attr_t::reduce_attr_t(reduce_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API reduce_attr_t::reduce_attr_t(const reduce_attr_t& src) :
        base_t(src)
{
}

CCL_API reduce_attr_t::reduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API reduce_attr_t::~reduce_attr_t()
{
}


/**
 * reduce_scatter coll attributes
 */
CCL_API reduce_scatter_attr_t::reduce_scatter_attr_t(reduce_scatter_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API reduce_scatter_attr_t::reduce_scatter_attr_t(const reduce_scatter_attr_t& src) :
        base_t(src)
{
}

CCL_API reduce_scatter_attr_t::reduce_scatter_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API reduce_scatter_attr_t::~reduce_scatter_attr_t()
{
}


/**
 * sparse_allreduce coll attributes
 */
CCL_API sparse_allreduce_attr_t::sparse_allreduce_attr_t(sparse_allreduce_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API sparse_allreduce_attr_t::sparse_allreduce_attr_t(const sparse_allreduce_attr_t& src) :
        base_t(src)
{
}

CCL_API sparse_allreduce_attr_t::sparse_allreduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
         base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API sparse_allreduce_attr_t::~sparse_allreduce_attr_t()
{
}

template <>
CCL_API const void * sparse_allreduce_attr_t::set<sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx, const void*>(const void *const & v)
{
    return get_impl()->set_attribute_value(v,
        details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx> {});
}
template <>
CCL_API const void *const & sparse_allreduce_attr_t::get<sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx>() const
{
    return get_impl()->get_attribute_value(
        details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx> {});
}


/**
 * barrier coll attributes
 */
CCL_API barrier_attr_t::barrier_attr_t(barrier_attr_t&& src) :
        base_t(std::move(src))
{
}

CCL_API barrier_attr_t::barrier_attr_t(const barrier_attr_t& src) :
        base_t(src)
{
}

CCL_API barrier_attr_t::barrier_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version) :
        base_t(std::shared_ptr<impl_t>(new impl_t(version)))
{
}

CCL_API barrier_attr_t::~barrier_attr_t()
{
}





/**
 * Force instantiations
 */
COMMON_API_FORCE_INSTANTIATION(allgatherv_attr_t)
COMMON_API_FORCE_INSTANTIATION(allreduce_attr_t)
COMMON_API_FORCE_INSTANTIATION(alltoall_attr_t)
COMMON_API_FORCE_INSTANTIATION(alltoallv_attr_t)
COMMON_API_FORCE_INSTANTIATION(bcast_attr_t)
COMMON_API_FORCE_INSTANTIATION(reduce_attr_t)
COMMON_API_FORCE_INSTANTIATION(reduce_scatter_attr_t)
COMMON_API_FORCE_INSTANTIATION(sparse_allreduce_attr_t)
COMMON_API_FORCE_INSTANTIATION(barrier_attr_t)

API_FORCE_INSTANTIATION(allgatherv_attr_t, allgatherv_op_attr_id, allgatherv_op_attr_id::vector_buf, int)
API_FORCE_INSTANTIATION(allreduce_attr_t, allreduce_op_attr_id, allreduce_op_attr_id::reduction_fn, ccl_reduction_fn_t)
API_FORCE_INSTANTIATION(reduce_attr_t, reduce_op_attr_id, reduce_op_attr_id::reduction_fn, ccl_reduction_fn_t)
API_FORCE_INSTANTIATION(reduce_scatter_attr_t, reduce_scatter_op_attr_id, reduce_scatter_op_attr_id::reduction_fn, ccl_reduction_fn_t)
API_FORCE_INSTANTIATION(sparse_allreduce_attr_t, sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_completion_fn, ccl_sparse_allreduce_completion_fn_t)
API_FORCE_INSTANTIATION(sparse_allreduce_attr_t, sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_alloc_fn, ccl_sparse_allreduce_alloc_fn_t)
//API_FORCE_INSTANTIATION(sparse_allreduce_attr_t, sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx, void*)
//template CCL_API const void * sparse_allreduce_attr_t::set<sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx, void*>(void *const & v);
API_FORCE_INSTANTIATION(sparse_allreduce_attr_t, sparse_allreduce_op_attr_id, sparse_allreduce_op_attr_id::sparse_coalesce_mode, ccl_sparse_coalesce_mode_t)


#undef API_FORCE_INSTANTIATION
#undef COMMON_API_FORCE_INSTANTIATION
#undef COMMA
}
