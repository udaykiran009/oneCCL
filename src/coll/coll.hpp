#pragma once

#include "common/comm/comm.hpp"
#include "common/stream/stream.hpp"
#include "common/datatype/datatype.hpp"
#include "common/utils/buffer.hpp"

#ifdef ENABLE_SYCL
#include <CL/sycl.hpp>
typedef cl::sycl::buffer<char, 1> ccl_sycl_buffer_t;
#endif /* ENABLE_SYCL */

#define CCL_INVALID_PROC_IDX (-1)

class ccl_sched;
class ccl_request;

enum ccl_coll_type
{
    ccl_coll_allgatherv,
    ccl_coll_allreduce,
    ccl_coll_barrier,
    ccl_coll_bcast,
    ccl_coll_reduce,
    ccl_coll_sparse_allreduce,
    ccl_coll_internal,
    ccl_coll_none,

    ccl_coll_last_value
};

struct ccl_coll_attr
{
    ccl_prologue_fn_t prologue_fn;
    ccl_epilogue_fn_t epilogue_fn;
    ccl_reduction_fn_t reduction_fn;
    size_t priority;
    int synchronous;
    int to_cache;
    std::string match_id;
};

struct ccl_coll_sparse_param
{
    const void* send_ind_buf;
    size_t send_val_count;
    const void* send_val_buf;
    size_t send_ind_count;
    void** recv_ind_buf;
    size_t* recv_ind_count;
    void** recv_val_buf;
    size_t* recv_val_count;
    ccl_datatype_internal_t itype;
};

struct ccl_coll_param
{
    ccl_coll_type ctype;
    void* buf;
    const void* send_buf;
    void* recv_buf;
    size_t count;
    size_t send_count;
    const size_t* recv_counts;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t reduction;
    size_t root;
    const ccl_stream* stream;
    ccl_comm* comm;
    ccl_coll_sparse_param sparse_param;
#ifdef ENABLE_SYCL
    ccl_sycl_buffer_t* sycl_send_buf;
    ccl_sycl_buffer_t* sycl_recv_buf;
    ccl_sycl_buffer_t* sycl_buf;
#endif /* ENABLE_SYCL */
};

enum ccl_coll_allgatherv_algo
{
    ccl_coll_allgatherv_direct,
    ccl_coll_allgatherv_naive,
    ccl_coll_allgatherv_flat,
    ccl_coll_allgatherv_multi_bcast,

    ccl_coll_allgatherv_last_value
};

enum ccl_coll_allreduce_algo
{
    ccl_coll_allreduce_direct,
    ccl_coll_allreduce_rabenseifner,
    ccl_coll_allreduce_starlike,
    ccl_coll_allreduce_ring,
    ccl_coll_allreduce_ring_rma,
    ccl_coll_allreduce_double_tree,
    ccl_coll_allreduce_recursive_doubling,

    ccl_coll_allreduce_last_value
};

enum ccl_coll_barrier_algo
{
    ccl_coll_barrier_direct,
    ccl_coll_barrier_ring,

    ccl_coll_barrier_last_value
};

enum ccl_coll_bcast_algo
{
    ccl_coll_bcast_direct,
    ccl_coll_bcast_ring,
    ccl_coll_bcast_double_tree,
    ccl_coll_bcast_naive,

    ccl_coll_bcast_last_value
};

enum ccl_coll_reduce_algo
{
    ccl_coll_reduce_direct,
    ccl_coll_reduce_tree,
    ccl_coll_reduce_double_tree,

    ccl_coll_reduce_last_value
};

enum ccl_coll_sparse_allreduce_algo
{
    ccl_coll_sparse_allreduce_basic,
    ccl_coll_sparse_allreduce_mask,

    ccl_coll_sparse_allreduce_last_value
};

const char* ccl_coll_type_to_str(ccl_coll_type type);

const char *ccl_coll_allgatherv_algo_to_str(ccl_coll_allgatherv_algo algo);
const char *ccl_coll_allreduce_algo_to_str(ccl_coll_allreduce_algo algo);
const char *ccl_coll_barrier_algo_to_str(ccl_coll_barrier_algo algo);
const char *ccl_coll_bcast_algo_to_str(ccl_coll_bcast_algo algo);
const char *ccl_coll_reduce_algo_to_str(ccl_coll_reduce_algo algo);

ccl_status_t ccl_coll_build_allgatherv(ccl_sched* sched,
                                       ccl_buffer send_buf,
                                       size_t send_count,
                                       ccl_buffer recv_buf,
                                       const size_t* recv_counts,
                                       ccl_datatype_internal_t dtype);

ccl_status_t ccl_coll_build_allreduce(ccl_sched* sched,
                                      ccl_buffer send_buf,
                                      ccl_buffer recv_buf,
                                      size_t count,
                                      ccl_datatype_internal_t dtype,
                                      ccl_reduction_t reduction);

ccl_status_t ccl_coll_build_barrier(ccl_sched* sched);

ccl_status_t ccl_coll_build_bcast(ccl_sched* sched,
                                  ccl_buffer buf,
                                  size_t count,
                                  ccl_datatype_internal_t dtype,
                                  size_t root);

ccl_status_t ccl_coll_build_reduce(ccl_sched* sched,
                                   ccl_buffer send_buf,
                                   ccl_buffer recv_buf,
                                   size_t count,
                                   ccl_datatype_internal_t dtype,
                                   ccl_reduction_t reduction,
                                   size_t root);

ccl_status_t ccl_coll_build_sparse_allreduce(ccl_sched* sched,
                                             ccl_buffer send_ind_buf, size_t send_ind_count,
                                             ccl_buffer send_val_buf, size_t send_val_count,
                                             ccl_buffer recv_ind_buf, size_t* recv_ind_count,
                                             ccl_buffer recv_val_buf, size_t* recv_val_count,
                                             ccl_datatype_internal_t index_dtype,
                                             ccl_datatype_internal_t value_dtype,
                                             ccl_reduction_t reduction);

ccl_request* ccl_allgatherv_impl(const void* send_buf,
                                 size_t send_count,
                                 void* recv_buf,
                                 const size_t* recv_counts,
                                 ccl_datatype_t dtype,
                                 const ccl_coll_attr_t* attributes,
                                 ccl_comm* communicator,
                                 const ccl_stream* stream);

ccl_request* ccl_allreduce_impl(const void* send_buf,
                                void* recv_buf,
                                size_t count,
                                ccl_datatype_t dtype,
                                ccl_reduction_t reduction,
                                const ccl_coll_attr_t* attributes,
                                ccl_comm* communicator,
                                const ccl_stream* stream);

void ccl_barrier_impl(ccl_comm* communicator,
                      const ccl_stream* stream);

ccl_request* ccl_bcast_impl(void* buf,
                            size_t count,
                            ccl_datatype_t dtype,
                            size_t root,
                            const ccl_coll_attr_t* attributes,
                            ccl_comm* communicator,
                            const ccl_stream* stream);

ccl_request* ccl_reduce_impl(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl_datatype_t dtype,
                             ccl_reduction_t reduction,
                             size_t root,
                             const ccl_coll_attr_t* attributes,
                             ccl_comm* communicator,
                             const ccl_stream* stream);

ccl_request* ccl_sparse_allreduce_impl(const void* send_ind_buf, size_t send_ind_count,
                                       const void* send_val_buf, size_t send_val_count,
                                       void** recv_ind_buf, size_t* recv_ind_count,
                                       void** recv_val_buf, size_t* recv_val_count,
                                       ccl_datatype_t index_dtype, ccl_datatype_t dtype,
                                       ccl_reduction_t reduction, const ccl_coll_attr_t* attributes,
                                       ccl_comm* communicator, const ccl_stream* stream);
