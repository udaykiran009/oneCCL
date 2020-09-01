#include "ccl_types.hpp"
#include "ccl_aliases.hpp"

#include "ccl_type_traits.hpp"
#include "ccl_types_policy.hpp"

#include "ccl_coll_attr_ids.hpp"
#include "ccl_coll_attr_ids_traits.hpp"
#include "ccl_coll_attr.hpp"

#include "ccl_comm_split_attr_ids.hpp"
#include "ccl_comm_split_attr_ids_traits.hpp"
#include "ccl_comm_split_attr.hpp"

#include "ccl_event_attr_ids.hpp"
#include "ccl_event_attr_ids_traits.hpp"
#include "ccl_event.hpp"

#include "ccl_stream_attr_ids.hpp"
#include "ccl_stream_attr_ids_traits.hpp"
#include "ccl_stream.hpp"

#include "common/request/request.hpp"

#include "common/comm/comm.hpp"
#include "coll/coll.hpp"
#include "coll/coll_common_attributes.hpp"
#include "coll/ccl_allgather_op_attr.hpp"
#include "coll/ccl_allreduce_op_attr.hpp"
#include "coll/ccl_alltoall_op_attr.hpp"
#include "coll/ccl_alltoallv_op_attr.hpp"
#include "coll/ccl_barrier_attr.hpp"
#include "coll/ccl_bcast_op_attr.hpp"
#include "coll/ccl_reduce_op_attr.hpp"
#include "coll/ccl_reduce_scatter_op_attr.hpp"
#include "coll/ccl_sparse_allreduce_op_attr.hpp"

#include "common/global/global.hpp"

#define COPY_COMMON_OP_ATTRS(from, to) \
    to->prologue_fn = from.get<ccl::common_op_attr_id::prolog_fn>().get(); \
    to->epilogue_fn = from.get<ccl::common_op_attr_id::epilog_fn>().get(); \
    to->priority = from.get<ccl::common_op_attr_id::priority>(); \
    to->synchronous = from.get<ccl::common_op_attr_id::synchronous>(); \
    to->to_cache = from.get<ccl::common_op_attr_id::to_cache>(); \
    to->match_id = from.get<ccl::common_op_attr_id::match_id>();

ccl_coll_attr::ccl_coll_attr(const ccl::allgatherv_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);

    vector_buf = attr.get<ccl::allgatherv_op_attr_id::vector_buf>();
}

ccl_coll_attr::ccl_coll_attr(const ccl::allreduce_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);

    reduction_fn = attr.get<ccl::allreduce_op_attr_id::reduction_fn>().get();
}

ccl_coll_attr::ccl_coll_attr(const ccl::alltoall_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);
}

ccl_coll_attr::ccl_coll_attr(const ccl::alltoallv_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);
}

ccl_coll_attr::ccl_coll_attr(const ccl::barrier_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);
}

ccl_coll_attr::ccl_coll_attr(const ccl::bcast_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);
}

ccl_coll_attr::ccl_coll_attr(const ccl::reduce_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);

    reduction_fn = attr.get<ccl::reduce_op_attr_id::reduction_fn>().get();
}

ccl_coll_attr::ccl_coll_attr(const ccl::reduce_scatter_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);

    reduction_fn = attr.get<ccl::reduce_scatter_op_attr_id::reduction_fn>().get();
}

ccl_coll_attr::ccl_coll_attr(const ccl::sparse_allreduce_attr_t& attr)
{
    COPY_COMMON_OP_ATTRS(attr, this);

    sparse_allreduce_completion_fn = attr.get<ccl::sparse_allreduce_op_attr_id::sparse_allreduce_completion_fn>().get();
    sparse_allreduce_alloc_fn = attr.get<ccl::sparse_allreduce_op_attr_id::sparse_allreduce_alloc_fn>().get();
    sparse_allreduce_fn_ctx = attr.get<ccl::sparse_allreduce_op_attr_id::sparse_allreduce_fn_ctx>();
    sparse_coalesce_mode = attr.get<ccl::sparse_allreduce_op_attr_id::sparse_coalesce_mode>();
}

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 const size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr& attr,
                                 ccl_comm* comm,
                                 const ccl_stream* stream)
{
    return nullptr;
}

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr& attr,
                                ccl_comm* comm,
                                const ccl_stream* stream)
{
    return nullptr;
}

ccl_request* ccl_alltoall_impl(const void* send_buf,
                               void* recv_buf,
                               size_t count,
                               ccl_datatype_t dtype,
                               const ccl_coll_attr& attr,
                               ccl_comm* comm,
                               const ccl_stream* stream)
{
    return nullptr;
}

ccl_request* ccl_alltoallv_impl(const void* send_buf,
                                const size_t* send_counts,
                                void* recv_buf,
                                const size_t* recv_counts,
                                ccl_datatype_t dtype,
                                const ccl_coll_attr& attr,
                                ccl_comm* comm,
                                const ccl_stream* stream)
{
    return nullptr;
}

void ccl_barrier_impl(ccl_comm* comm, const ccl_stream* stream)
{
}

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr& attr,
                            ccl_comm* comm,
                            const ccl_stream* stream)
{
    return nullptr;
}

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr& attr,
                             ccl_comm* comm,
                             const ccl_stream* stream)
{
    return nullptr;
}

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void* recv_ind_buf, size_t recv_ind_count,
                                       void* recv_val_buf, size_t recv_val_count,
                                       ccl_datatype_t index_dtype, ccl_datatype_t dtype,
                                       ccl_reduction_t reduction, const ccl_coll_attr& attr,
                                       ccl_comm* comm, const ccl_stream* stream)
{
    return nullptr;
}
