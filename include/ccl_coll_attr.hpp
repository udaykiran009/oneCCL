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
struct allgatherv_attr_t : public pointer_on_impl<allgatherv_attr_t, ccl_allgather_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<allgatherv_attr_t, ccl_allgather_op_attr_impl_t>::impl_value_t;

    ~allgatherv_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allgatherv_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <allgatherv_op_attr_id attrId>
    const typename allgatherv_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    allgatherv_attr_t(impl_value_t&& impl);
};

/**
 * Allreduce coll attributes
 */
struct allreduce_attr_t : public pointer_on_impl<allreduce_attr_t, ccl_allreduce_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<allreduce_attr_t, ccl_allreduce_op_attr_impl_t>::impl_value_t;

    ~allreduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allreduce_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <allreduce_op_attr_id attrId>
    const typename allreduce_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    allreduce_attr_t(impl_value_t&& impl);
};

/**
 * alltoall coll attributes
 */
struct alltoall_attr_t : public pointer_on_impl<alltoall_attr_t, ccl_alltoall_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<alltoall_attr_t, ccl_alltoall_op_attr_impl_t>::impl_value_t;

    ~alltoall_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <alltoall_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <alltoall_op_attr_id attrId>
    const typename alltoall_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    alltoall_attr_t(impl_value_t&& impl);
};

/**
 * Alltoallv coll attributes
 */
struct alltoallv_attr_t : public pointer_on_impl<alltoallv_attr_t, ccl_alltoallv_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<alltoallv_attr_t, ccl_alltoallv_op_attr_impl_t>::impl_value_t;

    ~alltoallv_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <alltoallv_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <alltoallv_op_attr_id attrId>
    const typename alltoallv_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    alltoallv_attr_t(impl_value_t&& impl);
};

/**
 * Bcast coll attributes
 */
struct bcast_attr_t : public pointer_on_impl<bcast_attr_t, ccl_bcast_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<bcast_attr_t, ccl_bcast_op_attr_impl_t>::impl_value_t;

    ~bcast_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <bcast_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <bcast_op_attr_id attrId>
    const typename bcast_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    bcast_attr_t(impl_value_t&& impl);
};

/**
 * Reduce coll attributes
 */
struct reduce_attr_t : public pointer_on_impl<reduce_attr_t, ccl_reduce_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<reduce_attr_t, ccl_reduce_op_attr_impl_t>::impl_value_t;

    ~reduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <reduce_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <reduce_op_attr_id attrId>
    const typename reduce_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    reduce_attr_t(impl_value_t&& impl);
};

/**
 * Reduce_scatter coll attributes
 */
struct reduce_scatter_attr_t : public pointer_on_impl<reduce_scatter_attr_t, ccl_reduce_scatter_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<reduce_scatter_attr_t, ccl_reduce_scatter_op_attr_impl_t>::impl_value_t;

    ~reduce_scatter_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <allreduce_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <reduce_scatter_op_attr_id attrId>
    const typename reduce_scatter_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    reduce_scatter_attr_t(impl_value_t&& impl);
};

/**
 * Sparse_allreduce coll attributes
 */
struct sparse_allreduce_attr_t : public pointer_on_impl<sparse_allreduce_attr_t, ccl_sparse_allreduce_op_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<sparse_allreduce_attr_t, ccl_sparse_allreduce_op_attr_impl_t>::impl_value_t;

    ~sparse_allreduce_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <sparse_allreduce_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <sparse_allreduce_op_attr_id attrId>
    const typename sparse_allreduce_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    sparse_allreduce_attr_t(impl_value_t&& impl);
};

/**
 * Barrier coll attributes
 */
struct barrier_attr_t : public pointer_on_impl<barrier_attr_t, ccl_barrier_attr_impl_t> {
    using impl_value_t =
        typename pointer_on_impl<barrier_attr_t, ccl_barrier_attr_impl_t>::impl_value_t;

    ~barrier_attr_t();

    /**
     * Set specific value for attribute by @attrId.
     * Previous attibute value would be returned
     */
    template <barrier_op_attr_id attrId,
              class Value,
              class = typename std::enable_if<is_attribute_value_supported<attrId, Value>()>::type>
    Value set_value(const Value& v);

    /**
     * Get specific attribute value by @attrId
     */
    template <barrier_op_attr_id attrId>
    const typename barrier_attr_traits<attrId>::type& get_value() const;

    /**
     * Get version of attributes
     */
    typename common_op_attr_traits<common_op_attr_id::version_id>::type get_version() const noexcept;

private:
    friend class environment;
    barrier_attr_t(impl_value_t&& impl);
};

} // namespace ccl
