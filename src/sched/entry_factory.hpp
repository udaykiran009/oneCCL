#pragma once

#include "sched/entry/entry.hpp"
#include "coll/coll.hpp"
#include "atl/atl.h"

#include <memory>
#include <list>
#include <functional>

class sync_object;

namespace out_of_order
{
class ooo_match;
}

class entry_factory
{
public:
    static std::shared_ptr<sched_entry> make_send_entry(ccl_sched* sched,
                                                        const ccl_buffer buf,
                                                        size_t cnt,
                                                        ccl_datatype_internal_t dtype,
                                                        size_t dst,
                                                        ccl_op_id_t op_id = 0);

    static std::shared_ptr<sched_entry> make_recv_entry(ccl_sched* sched,
                                                        ccl_buffer buf,
                                                        size_t cnt,
                                                        ccl_datatype_internal_t dtype,
                                                        size_t src,
                                                        ccl_op_id_t op_id = 0);

    static std::shared_ptr<sched_entry> make_write_entry(ccl_sched* sched,
                                                         ccl_buffer src_buf,
                                                         atl_mr_t* src_mr,
                                                         size_t cnt,
                                                         ccl_datatype_internal_t dtype,
                                                         size_t dst,
                                                         atl_mr_t* dst_mr,
                                                         size_t dst_buf_offset);

    static std::shared_ptr<sched_entry> make_reduce_local_entry(ccl_sched* sched,
                                                                const ccl_buffer in_buf,
                                                                size_t in_cnt,
                                                                ccl_buffer inout_buf,
                                                                size_t* out_cnt,
                                                                ccl_datatype_internal_t dtype,
                                                                ccl_reduction_t reduction_op);
    /**
     * Fused recv_reduce operation.
     * @param inout_buf Buffer with local data, will hold result of reduction.
     * @param in_cnt Number of elements in inout_buf for reduce operation and number of elements in comm_buf for receive operation
     * @param out_cnt Pointer to buffer to hold number of elements in inout_buf after reduce operation. Can be NULL
     * @param dtype Datatype of inout_buf and comm_buf
     * @param reduction_op Reduction operation, see @ref ccl_reduction_t
     * @param reduction_fn Pointer to the user provided custom reduction function
     * @param src Remote rank ID for receiving data
     * @param comm_buf Optional buffer for communication. Can be a @B nullptr, in that case CCL will allocate temporal buffer
     * @param op_id local operation id, used to construct atl tag
     */
    static std::shared_ptr<sched_entry> make_recv_reduce_entry(ccl_sched* sched,
                                                               ccl_buffer inout_buf,
                                                               size_t in_cnt,
                                                               size_t* out_cnt,
                                                               ccl_datatype_internal_t dtype,
                                                               ccl_reduction_t reduction_op,
                                                               size_t src,
                                                               ccl_buffer comm_buf,
                                                               ccl_op_id_t op_id = 0);

    static std::shared_ptr<sched_entry> make_copy_entry(ccl_sched* sched,
                                                        const ccl_buffer in_buf,
                                                        ccl_buffer out_buf,
                                                        size_t cnt,
                                                        ccl_datatype_internal_t dtype);

    static std::shared_ptr<sched_entry> make_sync_entry(ccl_sched* sched,
                                                        std::shared_ptr<sync_object> sync_obj);

    static std::shared_ptr<sched_entry> make_coll_entry(ccl_sched* sched,
                                                        ccl_coll_type coll_type,
                                                        const ccl_buffer send_buf,
                                                        ccl_buffer recv_buf, //buf for bcast
                                                        size_t cnt,
                                                        ccl_datatype_internal_t dtype,
                                                        ccl_reduction_t reduction_op,
                                                        size_t root = 0);

    static std::shared_ptr<sched_entry> make_prologue_entry(ccl_sched* sched,
                                                            ccl_prologue_fn_t fn,
                                                            const ccl_buffer in_buf,
                                                            size_t in_cnt,
                                                            ccl_datatype_internal_t in_dtype,
                                                            void** out_buf,
                                                            size_t* out_cnt,
                                                            ccl_datatype_t* out_dtype,
                                                            size_t* out_dtype_size);

    static std::shared_ptr<sched_entry> make_epilogue_entry(ccl_sched* sched,
                                                            ccl_epilogue_fn_t fn,
                                                            const ccl_buffer in_buf,
                                                            size_t in_cnt,
                                                            ccl_datatype_internal_t in_dtype,
                                                            ccl_buffer out_buf,
                                                            size_t expected_out_cnt,
                                                            ccl_datatype_internal_t out_dtype);

    static std::shared_ptr<sched_entry> make_wait_value_entry(ccl_sched* sched,
                                                              const volatile uint64_t* ptr,
                                                              uint64_t expected_value,
                                                              ccl_condition condition);

    static std::shared_ptr<sched_entry> make_function_entry(ccl_sched* sched,
                                                            ccl_sched_entry_function_t fn,
                                                            const void* ctx);

    static std::shared_ptr<sched_entry> make_register_entry(ccl_sched* sched,
                                                            size_t size,
                                                            const ccl_buffer ptr,
                                                            atl_mr_t** mr);

    static std::shared_ptr<sched_entry> make_deregister_entry(ccl_sched* sched,
                                                              std::list<atl_mr_t*>& mr_list);

    static std::shared_ptr<sched_entry> make_probe_entry(ccl_sched* sched,
                                                         size_t src,
                                                         size_t* cnt,
                                                         ccl_op_id_t op_id = 0);

    static std::shared_ptr<sched_entry> make_allreduce_entry(ccl_sched* sched,
                                                             const ccl_buffer send_buf,
                                                             ccl_buffer recv_buf,
                                                             size_t cnt,
                                                             ccl_datatype_internal_t dtype,
                                                             ccl_reduction_t reduction_op);

    static std::shared_ptr<sched_entry> make_allgatherv_entry(ccl_sched* sched,
                                                              const ccl_buffer send_buf,
                                                              size_t send_cnt,
                                                              ccl_buffer recv_buf,
                                                              size_t* recv_cnts,
                                                              ccl_datatype_internal_t dtype);

    static std::shared_ptr<sched_entry> make_bcast_entry(ccl_sched* sched,
                                                         ccl_buffer buf,
                                                         size_t cnt,
                                                         ccl_datatype_internal_t dtype,
                                                         size_t root);

    static std::shared_ptr<sched_entry> make_reduce_entry(ccl_sched *sched,
                                                          const ccl_buffer send_buf,
                                                          ccl_buffer recv_buf,
                                                          size_t cnt,
                                                          ccl_datatype_internal_t dtype,
                                                          ccl_reduction_t reduction,
                                                          size_t root);

    static std::shared_ptr<sched_entry> make_barrier_entry(ccl_sched *sched);

    static std::shared_ptr<sched_entry> make_chain_call_entry(ccl_sched* sched,
                                                              std::function<void(ccl_sched*)> sched_fill_function,
                                                              const char* entry_name = nullptr);

    static std::shared_ptr<sched_entry> make_nop_entry(ccl_sched* sched);
};
