#pragma once
#include "ccl_types.hpp"
#include "ccl_coll_attr.hpp"
#include "coll/coll_attributes.hpp"
#include "coll/ccl_allgather_op_attr_impl.hpp"
//#include "common/utils/tuple.hpp"

namespace ccl
{
    /* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class coll_attribute_type,
          class ...attr_value_pair_t>
coll_attribute_type create_coll_attr(attr_value_pair_t&&...avps)
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    auto coll_attr = coll_attribute_type(ret);

    int expander [] {(coll_attr.template set_value<attr_value_pair_t::idx()>(avps.val()), 0)...};
    return coll_attr;
}

/**
 *
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

template<allgatherv_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value allgatherv_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value allgatherv_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <allgatherv_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>::return_type& allgatherv_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& allgatherv_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * Allreduce attributes definition
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

template<allreduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type allreduce_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value allreduce_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <allreduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type& allreduce_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& allreduce_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * alltoall attributes definition
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

template<alltoall_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value alltoall_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value alltoall_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <alltoall_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>::return_type& alltoall_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& alltoall_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * alltoallv attributes definition
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

template<alltoallv_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value alltoallv_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value alltoallv_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <alltoallv_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>::return_type& alltoallv_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& alltoallv_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * bcast attributes definition
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

template<bcast_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value bcast_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value bcast_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <bcast_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>::return_type& bcast_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& bcast_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * reduce attributes definition
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

template<reduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value reduce_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value reduce_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <reduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>::return_type& reduce_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& reduce_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * reduce_scatter attributes definition
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

template<reduce_scatter_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value reduce_scatter_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value reduce_scatter_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <reduce_scatter_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>::return_type& reduce_scatter_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& reduce_scatter_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}


/**
 * sparse_allreduce attributes definition
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

template<sparse_allreduce_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value sparse_allreduce_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value sparse_allreduce_attr_t::set_value(const Value& v)
{
    return static_cast<ccl_common_op_attr_impl_t*>(get_impl().get())->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <sparse_allreduce_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>::return_type& sparse_allreduce_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& sparse_allreduce_attr_t::get_value() const
{
    return static_cast<const ccl_common_op_attr_impl_t*>(get_impl().get())->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}

/**
 * barrier attributes definition
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

template<barrier_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value barrier_attr_t::set_value(const Value& v)
{
    return get_impl()->set_attribute_value(v, details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId> {});
}

template<common_op_attr_id attrId,
             class Value/*,
             typename T*/>
CCL_API Value barrier_attr_t::set_value(const Value& v)
{
    return get_impl().get()->set_attribute_value(v, details::ccl_api_type_attr_traits<common_op_attr_id, attrId> {});
}

template <barrier_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>::return_type& barrier_attr_t::get_value() const
{
    return get_impl()->get_attribute_value(details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>{});
}

template <common_op_attr_id attrId>
CCL_API const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& barrier_attr_t::get_value() const
{
    return get_impl().get()->get_attribute_value(details::ccl_api_type_attr_traits<common_op_attr_id, attrId>{});
}
}
