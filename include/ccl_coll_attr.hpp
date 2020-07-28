#pragma once

#ifndef CCL_PRODUCT_FULL
#error "Do not include this file directly. Please include 'ccl.hpp'"
#endif

namespace ccl {

class ccl_allgather_op_attr_impl_t;
class ccl_allreduce_op_attr_impl_t;
class ccl_alltoall_op_attr_impl_t;
class ccl_alltoallv_op_attr_impl_t;
class ccl_bcast_op_attr_impl_t;
class ccl_reduce_op_attr_impl_t;
class ccl_reduce_scatter_op_attr_impl_t;
class ccl_sparse_allreduce_op_attr_impl_t;
class ccl_barrier_attr_impl_t;

/**
 * Allgatherv coll attributes
 */
class allgatherv_attr_t : public ccl_api_base_copyable<allgatherv_attr_t, copy_on_write_access_policy, ccl_allgather_op_attr_impl_t>
{
public:
    using base_t = ccl_api_base_copyable<allgatherv_attr_t, copy_on_write_access_policy, ccl_allgather_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~allgatherv_attr_t();

    /**
     * Set specific value for selft attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allgatherv_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);
    /**
     * Get specific attribute value by @attrId
     */
    template <allgatherv_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<allgatherv_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    allgatherv_attr_t(allgatherv_attr_t&& src);
    allgatherv_attr_t(const allgatherv_attr_t& src);
    allgatherv_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * Allreduce coll attributes
 */
class allreduce_attr_t : public ccl_api_base_copyable<allreduce_attr_t, copy_on_write_access_policy, ccl_allreduce_op_attr_impl_t> {

public:
    using base_t = ccl_api_base_copyable<allreduce_attr_t, copy_on_write_access_policy, ccl_allreduce_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~allreduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allreduce_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <allreduce_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<allreduce_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    allreduce_attr_t(allreduce_attr_t&& src);
    allreduce_attr_t(const allreduce_attr_t& src);
    allreduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * alltoall coll attributes
 */
struct alltoall_attr_t : public ccl_api_base_copyable<alltoall_attr_t, copy_on_write_access_policy, ccl_alltoall_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<alltoall_attr_t, copy_on_write_access_policy, ccl_alltoall_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~alltoall_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <alltoall_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <alltoall_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<alltoall_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    alltoall_attr_t(alltoall_attr_t&& src);
    alltoall_attr_t(const alltoall_attr_t& src);
    alltoall_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * Alltoallv coll attributes
 */
struct alltoallv_attr_t : public ccl_api_base_copyable<alltoallv_attr_t, copy_on_write_access_policy, ccl_alltoallv_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<alltoallv_attr_t, copy_on_write_access_policy, ccl_alltoallv_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~alltoallv_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <alltoallv_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <alltoallv_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<alltoallv_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    alltoallv_attr_t(alltoallv_attr_t&& src);
    alltoallv_attr_t(const alltoallv_attr_t& src);
    alltoallv_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * Bcast coll attributes
 */
struct bcast_attr_t : public ccl_api_base_copyable<bcast_attr_t, copy_on_write_access_policy, ccl_bcast_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<bcast_attr_t, copy_on_write_access_policy, ccl_bcast_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~bcast_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <bcast_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <bcast_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<bcast_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    bcast_attr_t(bcast_attr_t&& src);
    bcast_attr_t(const bcast_attr_t& src);
    bcast_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * Reduce coll attributes
 */
struct reduce_attr_t : public ccl_api_base_copyable<reduce_attr_t, copy_on_write_access_policy, ccl_reduce_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<reduce_attr_t, copy_on_write_access_policy, ccl_reduce_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~reduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <reduce_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <reduce_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<reduce_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    reduce_attr_t(reduce_attr_t&& src);
    reduce_attr_t(const reduce_attr_t& src);
    reduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);
};

/**
 * Reduce_scatter coll attributes
 */
struct reduce_scatter_attr_t : public ccl_api_base_copyable<reduce_scatter_attr_t, copy_on_write_access_policy, ccl_reduce_scatter_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<reduce_scatter_attr_t, copy_on_write_access_policy, ccl_reduce_scatter_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~reduce_scatter_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <reduce_scatter_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <reduce_scatter_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<reduce_scatter_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    reduce_scatter_attr_t(reduce_scatter_attr_t&& src);
    reduce_scatter_attr_t(const reduce_scatter_attr_t& src);
    reduce_scatter_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);;
};

/**
 * Sparse_allreduce coll attributes
 */
struct sparse_allreduce_attr_t : public ccl_api_base_copyable<sparse_allreduce_attr_t, copy_on_write_access_policy, ccl_sparse_allreduce_op_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<sparse_allreduce_attr_t, copy_on_write_access_policy, ccl_sparse_allreduce_op_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~sparse_allreduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <sparse_allreduce_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <sparse_allreduce_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<sparse_allreduce_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    sparse_allreduce_attr_t(sparse_allreduce_attr_t&& src);
    sparse_allreduce_attr_t(const sparse_allreduce_attr_t& src);
    sparse_allreduce_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);;
};

/**
 * Barrier coll attributes
 */
struct barrier_attr_t : public ccl_api_base_copyable<barrier_attr_t, copy_on_write_access_policy, ccl_barrier_attr_impl_t> {
public:
    using base_t = ccl_api_base_copyable<barrier_attr_t, copy_on_write_access_policy, ccl_barrier_attr_impl_t>;

    /**
     * Declare PIMPL type
     */
    using impl_value_t = typename base_t::impl_value_t;

    /**
     * Declare implementation type
     */
    using impl_t = typename impl_value_t::element_type;

    ~barrier_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <barrier_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    template <common_op_attr_id attrId,
              class Value/*,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type*/>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <barrier_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<barrier_op_attr_id, attrId>::return_type& get_value() const;

    template <common_op_attr_id attrId>
    const typename details::ccl_api_type_attr_traits<common_op_attr_id, attrId>::return_type& get_value() const;

private:
    friend class environment;
    barrier_attr_t(barrier_attr_t&& src);
    barrier_attr_t(const barrier_attr_t& src);
    barrier_attr_t(const typename details::ccl_api_type_attr_traits<common_op_attr_id, common_op_attr_id::version>::type& version);;
};


template <allgatherv_op_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<allgatherv_op_attr_id,
                                                              t, value_type>
{
    return details::attr_value_tripple<allgatherv_op_attr_id, t, value_type>(v);
}

template <allreduce_op_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<allreduce_op_attr_id,
                                                              t, value_type>
{
    return details::attr_value_tripple<allreduce_op_attr_id, t, value_type>(v);
}

template <common_op_attr_id t, class value_type>
constexpr auto attr_arg(value_type v) -> details::attr_value_tripple<common_op_attr_id, t, value_type>
{
    return details::attr_value_tripple<common_op_attr_id, t, value_type>(v);
}

/* TODO temporary function for UT compilation: would be part of ccl::environment in final*/
template <class coll_attribute_type,
          class ...attr_value_pair_t>
coll_attribute_type create_coll_attr(attr_value_pair_t&&...avps);
} // namespace ccl
