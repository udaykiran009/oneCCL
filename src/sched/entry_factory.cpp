#include "sched/entry_factory.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry_types/send_entry.hpp"
#include "sched/entry_types/recv_entry.hpp"
#include "sched/entry_types/write_entry.hpp"
#include "sched/entry_types/reduce_entry.hpp"
#include "sched/entry_types/recv_reduce_entry.hpp"
#include "sched/entry_types/copy_entry.hpp"
#include "sched/entry_types/sync_entry.hpp"
#include "sched/entry_types/coll_entry.hpp"
#include "sched/entry_types/prologue_entry.hpp"
#include "sched/entry_types/epilogue_entry.hpp"
#include "sched/entry_types/postponed_fields.hpp"
#include "sched/entry_types/tensor_comm_entry.hpp"
#include "sched/entry_types/wait_value_entry.hpp"
#include "sched/entry_types/function_entry.hpp"
#include "sched/entry_types/register_entry.hpp"
#include "sched/entry_types/deregister_entry.hpp"
#include "sched/entry_types/nop_entry.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"

#include <memory>

std::shared_ptr<sched_entry> entry_factory::make_send_entry(mlsl_sched* sched,
                                                            const void* buffer,
                                                            size_t count,
                                                            mlsl_datatype_internal_t data_type,
                                                            size_t destination)
{
    MLSL_LOG(DEBUG, "creating SEND entry");
    auto e = std::make_shared<send_entry>(sched, buffer, count, data_type,
                                          global_data.comm->get_global_rank(destination), global_data.comm->rank());
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_entry(mlsl_sched* sched,
                                                            void* buffer,
                                                            size_t count,
                                                            mlsl_datatype_internal_t data_type,
                                                            size_t source)
{
    MLSL_LOG(DEBUG, "creating RECV entry");
    auto e = std::make_shared<recv_entry>(sched, buffer, count, data_type, global_data.comm->get_global_rank(source));
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_write_entry(mlsl_sched* sched,
                                                             const void* src_buf,
                                                             atl_mr_t* src_mr,
                                                             size_t cnt,
                                                             mlsl_datatype_internal_t dtype,
                                                             size_t dst,
                                                             atl_mr_t* dst_mr,
                                                             size_t dst_buf_offset)
{
    MLSL_LOG(DEBUG, "creating WRITE entry");
    auto e = std::make_shared<write_entry>(sched, src_buf, src_mr, cnt, dtype,
                                           dst, dst_mr, dst_buf_offset);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_reduce_entry(mlsl_sched* sched,
                                                              const void* in_buffer,
                                                              size_t in_count,
                                                              void* inout_buffer,
                                                              size_t* out_count,
                                                              mlsl_datatype_internal_t data_type,
                                                              mlsl_reduction_t reduction_op)
{
    MLSL_LOG(DEBUG, "creating REDUCE entry");
    auto e = std::make_shared<reduce_entry>(sched, in_buffer, in_count, inout_buffer, out_count, data_type,
                                            reduction_op,
                                            sched->coll_attr.reduction_fn);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_reduce_entry(mlsl_sched* sched,
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
    auto e = std::make_shared<recv_reduce_entry>(sched, inout_buffer, count, out_count, data_type, reduction_op,
                                                 reduction_fn,
                                                 global_data.comm->get_global_rank(source), communication_buf);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_copy_entry(mlsl_sched* sched,
                                                            const void* in_buffer,
                                                            void* out_buffer,
                                                            size_t count,
                                                            mlsl_datatype_internal_t data_type)
{
    MLSL_LOG(DEBUG, "creating COPY entry");
    auto e = std::make_shared<copy_entry>(sched, in_buffer, out_buffer, count, data_type);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_sync_entry(mlsl_sched* sched,
                                                            std::shared_ptr<sync_object> sync_obj)
{
    MLSL_LOG(DEBUG, "creating SYNC entry");
    auto e = std::make_shared<sync_entry>(sched, sync_obj);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_coll_entry(mlsl_sched* sched,
                                                            mlsl_coll_type coll_type,
                                                            const void* send_buffer,
                                                            void* recv_buffer,
                                                            size_t count,
                                                            mlsl_datatype_internal_t data_type,
                                                            mlsl_reduction_t reduction_op)
{
    MLSL_LOG(DEBUG, "creating COLLECTIVE entry");
    auto e = std::make_shared<coll_entry>(sched, coll_type, send_buffer, recv_buffer, count, data_type,
                                          reduction_op);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_prologue_entry(mlsl_sched* sched,
                                                                mlsl_prologue_fn_t prologue_fn,
                                                                const void* in_buffer,
                                                                size_t in_count,
                                                                mlsl_datatype_internal_t in_data_type,
                                                                void** out_buffer,
                                                                size_t* out_count,
                                                                mlsl_datatype_t* out_data_type,
                                                                size_t* out_dtype_size)
{
    MLSL_LOG(DEBUG, "creating PROLOGUE entry");
    auto e = std::make_shared<prologue_entry>(sched, prologue_fn, in_buffer, in_count, in_data_type,
                                              out_buffer, out_count, out_data_type, out_dtype_size);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_epilogue_entry(mlsl_sched* sched,
                                                                mlsl_epilogue_fn_t epilogue_fn,
                                                                const void* in_buffer,
                                                                size_t in_count,
                                                                mlsl_datatype_internal_t in_data_type,
                                                                void* out_buffer,
                                                                size_t expected_out_count,
                                                                mlsl_datatype_internal_t out_data_type)
{
    MLSL_LOG(DEBUG, "creating EPILOGUE entry");
    auto e = std::make_shared<epilogue_entry>(sched, epilogue_fn, in_buffer, in_count, in_data_type, out_buffer,
                                              expected_out_count, out_data_type);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_tensor_comm_entry(mlsl_sched* sched,
                                                                   out_of_order::ooo_match* ooo_handler,
                                                                   const char* tensor_name)
{
    MLSL_LOG(DEBUG, "creating TENSOR_COMM entry");
    auto e = std::make_shared<tensor_comm_entry>(sched, ooo_handler, tensor_name);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_wait_value_entry(mlsl_sched* sched,
                                                                  const volatile uint64_t* ptr,
                                                                  uint64_t expected_value,
                                                                  mlsl_condition condition)
{
    MLSL_LOG(DEBUG, "creating WAIT_VALUE entry");
    auto e = std::make_shared<wait_value_entry>(sched, ptr, expected_value, condition);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_function_entry(mlsl_sched* sched,
                                                                mlsl_sched_entry_function_t fn,
                                                                const void* ctx)
{
    MLSL_LOG(DEBUG, "creating FUNCTION entry");
    auto e = std::make_shared<function_entry>(sched, fn, ctx);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_register_entry(mlsl_sched* sched,
                                                                size_t size,
                                                                void* ptr,
                                                                atl_mr_t** mr)
{
    MLSL_LOG(DEBUG, "creating REGISTER entry");
    auto e = std::make_shared<register_entry>(sched, size, ptr, mr);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_deregister_entry(mlsl_sched* sched,
                                                                  std::list<atl_mr_t*>& mr_list)
{
    MLSL_LOG(DEBUG, "creating DEREGISTER entry");
    auto e = std::make_shared<deregister_entry>(sched, mr_list);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_nop_entry(mlsl_sched* sched)
{
    MLSL_LOG(DEBUG, "creating TENSOR_COMM entry");
    auto e = std::make_shared<nop_entry>(sched);
    sched->add_entry(e);
    return e;
}
