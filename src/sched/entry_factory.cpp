#include "sched/entry_factory.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_types/send_entry.hpp"
#include "sched/entry_types/recv_entry.hpp"
#include "sched/entry_types/reduce_entry.hpp"
#include "sched/entry_types/recv_reduce_entry.hpp"
#include "sched/entry_types/copy_entry.hpp"
#include "sched/entry_types/sync_entry.hpp"
#include "sched/entry_types/collective_entry.hpp"
#include "sched/entry_types/prologue_entry.hpp"
#include "sched/entry_types/epilogue_entry.hpp"
#include "sched/entry_types/postponed_fields_entry.hpp"
#include "sched/entry_types/tensor_comm_entry.hpp"
#include "sched/entry_types/nop_entry.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"

#include <memory>

std::shared_ptr<sched_entry> entry_factory::make_send_entry(mlsl_sched* schedule,
                                                           const void* buffer,
                                                           size_t count,
                                                           mlsl_datatype_internal_t data_type,
                                                           size_t destination)
{
    MLSL_LOG(DEBUG, "creating SEND entry");
    return std::make_shared<send_entry>(schedule, buffer, count, data_type, destination, global_data.comm);
}

std::shared_ptr<sched_entry> entry_factory::make_recv_entry(mlsl_sched* schedule,
                                                           void* buffer,
                                                           size_t count,
                                                           mlsl_datatype_internal_t data_type,
                                                           size_t source)
{
    MLSL_LOG(DEBUG, "creating RECV entry");
    return std::make_shared<recv_entry>(schedule, buffer, count, data_type, source, global_data.comm);
}

std::shared_ptr<sched_entry> entry_factory::make_reduce_entry(const void* in_buffer,
                                                              size_t in_count,
                                                              void* inout_buffer,
                                                              size_t* out_count,
                                                              mlsl_datatype_internal_t data_type,
                                                              mlsl_reduction_t reduction_op,
                                                              mlsl_reduction_fn_t reduction_fn)
{
    MLSL_LOG(DEBUG, "creating REDUCE entry");
    return std::make_shared<reduce_entry>(in_buffer, in_count, inout_buffer, out_count, data_type, reduction_op,
                                        reduction_fn);

}

std::shared_ptr<sched_entry> entry_factory::make_recv_reduce_entry(mlsl_sched* schedule,
                                                                  void* inout_buffer,
                                                                  size_t count,
                                                                  size_t* out_count,
                                                                  mlsl_datatype_internal_t data_type,
                                                                  mlsl_reduction_t reduction_op,
                                                                  mlsl_reduction_fn_t reduction_fn,
                                                                  size_t source,
                                                                  void* communication_buf)
{
    MLSL_LOG(DEBUG, "creating RECV_REDUCE entry");
    return std::make_shared<recv_reduce_entry>(schedule, inout_buffer, count, out_count, data_type, reduction_op, reduction_fn,
                                               source, communication_buf, global_data.comm);
}

std::shared_ptr<sched_entry> entry_factory::make_copy_entry(mlsl_sched* schedule,
                                                           const void* in_buffer,
                                                           void* out_buffer,
                                                           size_t count,
                                                           mlsl_datatype_internal_t data_type)
{
    MLSL_LOG(DEBUG, "creating COPY entry");
    return std::make_shared<copy_entry>(schedule, in_buffer, out_buffer, count, data_type);
}

std::shared_ptr<sched_entry> entry_factory::make_sync_entry(std::shared_ptr<sync_object> sync_obj)
{
    MLSL_LOG(DEBUG, "creating SYNC entry");
    return std::make_shared<sync_entry>(sync_obj);
}

std::shared_ptr<sched_entry> entry_factory::make_coll_entry(mlsl_sched* schedule,
                                                           mlsl_coll_type coll_type,
                                                           const void* send_buffer,
                                                           void* recv_buffer,
                                                           size_t count,
                                                           mlsl_datatype_internal_t data_type,
                                                           mlsl_reduction_t reduction_op,
                                                           mlsl_comm* communicator)
{
    MLSL_LOG(DEBUG, "creating COLLECTIVE entry");
    return std::make_shared<collective_entry>(schedule, coll_type, send_buffer, recv_buffer, count, data_type, reduction_op,
                                              communicator);
}

std::shared_ptr<sched_entry> entry_factory::make_prologue_entry(mlsl_prologue_fn_t prologue_fn,
                                                                const void* in_buffer,
                                                                size_t in_count,
                                                                mlsl_datatype_internal_t in_data_type,
                                                                void** out_buffer,
                                                                size_t* out_count,
                                                                mlsl_datatype_internal* out_data_type)
{
    MLSL_LOG(DEBUG, "creating PROLOGUE entry");
    return std::make_shared<prologue_entry>(prologue_fn, in_buffer, in_count, in_data_type, out_buffer,
                                            out_count, out_data_type);
}

std::shared_ptr<sched_entry>
entry_factory::make_epilogue_entry(mlsl_sched* schedule,
                                  mlsl_epilogue_fn_t epilogue_fn,
                                  const void* in_buffer,
                                  size_t in_count,
                                  mlsl_datatype_internal_t in_data_type,
                                  void* out_buffer,
                                  size_t* out_count,
                                  size_t expected_out_count,
                                  mlsl_datatype_internal* out_data_type)
{
    MLSL_LOG(DEBUG, "creating EPILOGUE entry");
    return std::make_shared<epilogue_entry>(schedule, epilogue_fn, in_buffer, in_count, in_data_type, out_buffer,
                                            out_count, expected_out_count, out_data_type);
}

std::shared_ptr<sched_entry> entry_factory::make_postponed_fields_entry(mlsl_sched* schedule,
                                                                       size_t part_idx,
                                                                       size_t part_count)
{
    MLSL_LOG(DEBUG, "creating UPDATE_FIELDS entry");
    return std::make_shared<postponed_fields_entry>(schedule, part_idx, part_count);
}

std::shared_ptr<sched_entry> entry_factory::make_tensor_comm_entry(out_of_order::ooo_match* ooo_handler,
                                                                   const char* tensor_name)
{
    MLSL_LOG(DEBUG, "creating TENSOR_COMM entry");
    return std::make_shared<tensor_comm_entry>(ooo_handler, tensor_name);
}

std::shared_ptr<sched_entry> entry_factory::make_nop_entry()
{
    MLSL_LOG(DEBUG, "creating TENSOR_COMM entry");
    return std::make_shared<nop_entry>();
}
