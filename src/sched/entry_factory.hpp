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
    static std::shared_ptr<sched_entry> make_send_entry(mlsl_sched* schedule,
                                                        const void* buffer,
                                                        size_t count,
                                                        mlsl_datatype_internal_t data_type,
                                                        size_t destination);

    static std::shared_ptr<sched_entry> make_recv_entry(mlsl_sched* schedule,
                                                        void* buffer,
                                                        size_t count,
                                                        mlsl_datatype_internal_t data_type,
                                                        size_t source);

    static std::shared_ptr<sched_entry> make_reduce_entry(const void* in_buffer,
                                                          size_t in_count,
                                                          void* inout_buffer,
                                                          size_t* out_count,
                                                          mlsl_datatype_internal_t data_type,
                                                          mlsl_reduction_t reduction_op,
                                                          mlsl_reduction_fn_t reduction_fn);

    /**
     * Combination of recv and reduce operations.
     * @param inout_buf Buffer with local data, will hold result of reduction.
     * @param count Number of elements in inout_buf for reduce operation and number of elements in comm_buf for receive operation
     * @param out_count Pointer to buffer to hold number of elements in inout_buf after reduce operation. Can be NULL
     * @param data_type Data type of inout_buf and comm_buf
     * @param reduction_op Reduction operation, see @ref mlsl_reduction_t
     * @param reduction_fn Pointer to the user provided custom reduction function
     * @param source Remote rank ID for receiving data
     * @param communication_buf Optional buffer for communication. Can be a @B NULL, in that case MLSL will allocate temporal buffer
     */
    static std::shared_ptr<sched_entry> make_recv_reduce_entry(mlsl_sched* schedule,
                                                               void* inout_buffer,
                                                               size_t count,
                                                               size_t* out_count,
                                                               mlsl_datatype_internal_t data_type,
                                                               mlsl_reduction_t reduction_op,
                                                               mlsl_reduction_fn_t reduction_fn,
                                                               size_t source,
                                                               void* communication_buf);

    static std::shared_ptr<sched_entry> make_copy_entry(mlsl_sched* schedule,
                                                        const void* in_buffer,
                                                        void* out_buffer,
                                                        size_t count,
                                                        mlsl_datatype_internal_t data_type);

    static std::shared_ptr<sched_entry> make_sync_entry(std::shared_ptr<sync_object> sync_obj);

    static std::shared_ptr<sched_entry> make_coll_entry(mlsl_sched* schedule,
                                                        mlsl_coll_type coll_type,
                                                        const void* send_buffer,
                                                        void* recv_buffer,
                                                        size_t count,
                                                        mlsl_datatype_internal_t data_type,
                                                        mlsl_reduction_t reduction_op,
                                                        mlsl_comm* communicator);

    static std::shared_ptr<sched_entry> make_prologue_entry(mlsl_prologue_fn_t prologue_fn,
                                                            const void* in_buffer,
                                                            size_t in_count,
                                                            mlsl_datatype_internal_t in_data_type,
                                                            void** out_buffer,
                                                            size_t* out_count,
                                                            mlsl_datatype_internal* out_data_type);

    static std::shared_ptr<sched_entry> make_epilogue_entry(mlsl_sched* schedule,
                                                            mlsl_epilogue_fn_t epilogue_fn,
                                                            const void* in_buffer,
                                                            size_t in_count,
                                                            mlsl_datatype_internal_t in_data_type,
                                                            void* out_buffer,
                                                            size_t* out_count,
                                                            size_t expected_out_count,
                                                            mlsl_datatype_internal* out_data_type);

    static std::shared_ptr<sched_entry> make_postponed_fields_entry(mlsl_sched* schedule,
                                                                    size_t part_idx,
                                                                    size_t part_count);

    static std::shared_ptr<sched_entry> make_tensor_comm_entry(out_of_order::ooo_match* ooo_handler,
                                                               const char* tensor_name);

    static std::shared_ptr<sched_entry> make_nop_entry();
};
