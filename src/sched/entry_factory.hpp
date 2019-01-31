#pragma once

#include "sched/entry_types/entry.hpp"
#include "coll/coll.hpp"

#include <memory>

class sync_object;

namespace out_of_order
{
class ooo_match;
}

class entry_factory
{
public:
    static std::shared_ptr<sched_entry> make_send_entry(mlsl_sched* sched,
                                                        const void* buf,
                                                        size_t cnt,
                                                        mlsl_datatype_internal_t dtype,
                                                        size_t dst);

    static std::shared_ptr<sched_entry> make_recv_entry(mlsl_sched* sched,
                                                        void* buf,
                                                        size_t cnt,
                                                        mlsl_datatype_internal_t dtype,
                                                        size_t src);

    static std::shared_ptr<sched_entry> make_write_entry(mlsl_sched* sched,
                                                         const void* src_buf,
                                                         atl_mr_t* src_mr,
                                                         size_t cnt,
                                                         mlsl_datatype_internal_t dtype,
                                                         size_t dst,
                                                         atl_mr_t* dst_mr,
                                                         size_t dst_buf_offset);

    static std::shared_ptr<sched_entry> make_reduce_entry(mlsl_sched* sched,
                                                          const void* in_buf,
                                                          size_t in_cnt,
                                                          void* inout_buf,
                                                          size_t* out_cnt,
                                                          mlsl_datatype_internal_t dtype,
                                                          mlsl_reduction_t reduction_op);

    /**
     * Fused recv_reduce operation.
     * @param inout_buf Buffer with local data, will hold result of reduction.
     * @param in_cnt Number of elements in inout_buf for reduce operation and number of elements in comm_buf for receive operation
     * @param out_cnt Pointer to buffer to hold number of elements in inout_buf after reduce operation. Can be NULL
     * @param dtype Datatype of inout_buf and comm_buf
     * @param reduction_op Reduction operation, see @ref mlsl_reduction_t
     * @param reduction_fn Pointer to the user provided custom reduction function
     * @param src Remote rank ID for receiving data
     * @param comm_buf Optional buffer for communication. Can be a @B NULL, in that case MLSL will allocate temporal buffer
     */
    static std::shared_ptr<sched_entry> make_recv_reduce_entry(mlsl_sched* sched,
                                                               void* inout_buf,
                                                               size_t in_cnt,
                                                               size_t* out_cnt,
                                                               mlsl_datatype_internal_t dtype,
                                                               mlsl_reduction_t reduction_op,
                                                               size_t src,
                                                               void* comm_buf);

    static std::shared_ptr<sched_entry> make_copy_entry(mlsl_sched* sched,
                                                        const void* in_buf,
                                                        void* out_buf,
                                                        size_t cnt,
                                                        mlsl_datatype_internal_t dtype);

    static std::shared_ptr<sched_entry> make_sync_entry(mlsl_sched* sched,
                                                        std::shared_ptr<sync_object> sync_obj);

    static std::shared_ptr<sched_entry> make_coll_entry(mlsl_sched* sched,
                                                        mlsl_coll_type coll_type,
                                                        const void* send_buf,
                                                        void* recv_buf,
                                                        size_t cnt,
                                                        mlsl_datatype_internal_t dtype,
                                                        mlsl_reduction_t reduction_op);

    static std::shared_ptr<sched_entry> make_prologue_entry(mlsl_sched* sched,
                                                            mlsl_prologue_fn_t fn,
                                                            const void* in_buf,
                                                            size_t in_cnt,
                                                            mlsl_datatype_internal_t in_dtype,
                                                            void** out_buf,
                                                            size_t* out_cnt,
                                                            mlsl_datatype_t* out_dtype,
                                                            size_t* out_dtype_size);

    static std::shared_ptr<sched_entry> make_epilogue_entry(mlsl_sched* sched,
                                                            mlsl_epilogue_fn_t fn,
                                                            const void* in_buf,
                                                            size_t in_cnt,
                                                            mlsl_datatype_internal_t in_dtype,
                                                            void* out_buf,
                                                            size_t expected_out_cnt,
                                                            mlsl_datatype_internal_t out_dtype);

    static std::shared_ptr<sched_entry> make_tensor_comm_entry(mlsl_sched* sched,
                                                               out_of_order::ooo_match* ooo_handler,
                                                               const char* tensor_name);

    static std::shared_ptr<sched_entry> make_wait_value_entry(mlsl_sched* sched,
                                                              const volatile uint64_t* ptr,
                                                              uint64_t expected_value,
                                                              mlsl_condition condition);

    static std::shared_ptr<sched_entry> make_function_entry(mlsl_sched* sched,
                                                            mlsl_sched_entry_function_t fn,
                                                            const void* ctx);

    static std::shared_ptr<sched_entry> make_register_entry(mlsl_sched* sched,
                                                            size_t size,
                                                            void* ptr,
                                                            atl_mr_t** mr);

    static std::shared_ptr<sched_entry> make_nop_entry(mlsl_sched* sched);
};
