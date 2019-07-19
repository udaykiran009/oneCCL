#include "sched/sched.hpp"
#include "sched/entry_factory.hpp"
#include "sched/sync_object.hpp"
#include "sched/entry/send_entry.hpp"
#include "sched/entry/recv_entry.hpp"
#include "sched/entry/write_entry.hpp"
#include "sched/entry/reduce_local_entry.hpp"
#include "sched/entry/recv_reduce_entry.hpp"
#include "sched/entry/copy_entry.hpp"
#include "sched/entry/sync_entry.hpp"
#include "sched/entry/prologue_entry.hpp"
#include "sched/entry/epilogue_entry.hpp"
#include "sched/entry/wait_value_entry.hpp"
#include "sched/entry/function_entry.hpp"
#include "sched/entry/probe_entry.hpp"
#include "sched/entry/register_entry.hpp"
#include "sched/entry/deregister_entry.hpp"
#include "sched/entry/chain_call_entry.hpp"
#include "sched/entry/nop_entry.hpp"
#include "sched/entry/coll/coll_entry.hpp"
#include "sched/entry/coll/allreduce_entry.hpp"
#include "sched/entry/coll/allgatherv_entry.hpp"
#include "sched/entry/coll/bcast_entry.hpp"
#include "sched/entry/coll/reduce_entry.hpp"
#include "sched/entry/coll/barrier_entry.hpp"

std::shared_ptr<sched_entry> entry_factory::make_send_entry(iccl_sched* sched,
                                                            const iccl_buffer buf,
                                                            size_t cnt,
                                                            iccl_datatype_internal_t dtype,
                                                            size_t dst,
                                                            iccl_op_id_t op_id)
{
    auto e = std::make_shared<send_entry>(sched, buf, cnt, dtype,
                                          sched->coll_param.comm->get_global_rank(dst), global_data.comm->rank(),
                                          op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_entry(iccl_sched* sched,
                                                            iccl_buffer buf,
                                                            size_t cnt,
                                                            iccl_datatype_internal_t dtype,
                                                            size_t src,
                                                            iccl_op_id_t op_id)
{
    auto e = std::make_shared<recv_entry>(sched, buf, cnt, dtype, sched->coll_param.comm->get_global_rank(src), op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_write_entry(iccl_sched* sched,
                                                             iccl_buffer src_buf,
                                                             atl_mr_t* src_mr,
                                                             size_t cnt,
                                                             iccl_datatype_internal_t dtype,
                                                             size_t dst,
                                                             atl_mr_t* dst_mr,
                                                             size_t dst_buf_offset)
{
    auto e = std::make_shared<write_entry>(sched, src_buf, src_mr, cnt, dtype,
                                           dst, dst_mr, dst_buf_offset);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_reduce_local_entry(iccl_sched* sched,
                                                                    const iccl_buffer in_buf,
                                                                    size_t in_cnt,
                                                                    iccl_buffer inout_buf,
                                                                    size_t* out_cnt,
                                                                    iccl_datatype_internal_t dtype,
                                                                    iccl_reduction_t reduction_op)
{
    auto e = std::make_shared<reduce_local_entry>(sched, in_buf, in_cnt, inout_buf,
                                                  out_cnt, dtype, reduction_op);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_reduce_entry(iccl_sched* sched,
                                                                   iccl_buffer inout_buf,
                                                                   size_t in_cnt,
                                                                   size_t* out_cnt,
                                                                   iccl_datatype_internal_t dtype,
                                                                   iccl_reduction_t reduction_op,
                                                                   size_t src,
                                                                   iccl_buffer comm_buf,
                                                                   iccl_op_id_t op_id)
{
    auto e = std::make_shared<recv_reduce_entry>(sched, inout_buf, in_cnt, out_cnt, dtype, reduction_op,
                                                 sched->coll_param.comm->get_global_rank(src), comm_buf, op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_copy_entry(iccl_sched* sched,
                                                            const iccl_buffer in_buf,
                                                            iccl_buffer out_buf,
                                                            size_t cnt,
                                                            iccl_datatype_internal_t dtype)
{
    auto e = std::make_shared<copy_entry>(sched, in_buf, out_buf, cnt, dtype);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_sync_entry(iccl_sched* sched,
                                                            std::shared_ptr<sync_object> sync_obj)
{
    auto e = std::make_shared<sync_entry>(sched, sync_obj);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_coll_entry(iccl_sched* sched,
                                                            iccl_coll_type coll_type,
                                                            const iccl_buffer send_buf,
                                                            iccl_buffer recv_buf,
                                                            size_t cnt,
                                                            iccl_datatype_internal_t dtype,
                                                            iccl_reduction_t reduction_op,
                                                            size_t root)
{
    auto e = std::make_shared<coll_entry>(sched, coll_type, send_buf, recv_buf,
                                          cnt, dtype, reduction_op, root);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_prologue_entry(iccl_sched* sched,
                                                                iccl_prologue_fn_t fn,
                                                                const iccl_buffer in_buf,
                                                                size_t in_cnt,
                                                                iccl_datatype_internal_t in_dtype,
                                                                void** out_buf,
                                                                size_t* out_cnt,
                                                                iccl_datatype_t* out_dtype,
                                                                size_t* out_dtype_size)
{
    auto e = std::make_shared<prologue_entry>(sched, fn, in_buf, in_cnt, in_dtype,
                                              out_buf, out_cnt, out_dtype, out_dtype_size);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_epilogue_entry(iccl_sched* sched,
                                                                iccl_epilogue_fn_t fn,
                                                                const iccl_buffer in_buf,
                                                                size_t in_cnt,
                                                                iccl_datatype_internal_t in_dtype,
                                                                iccl_buffer out_buf,
                                                                size_t expected_out_cnt,
                                                                iccl_datatype_internal_t out_dtype)
{
    auto e = std::make_shared<epilogue_entry>(sched, fn, in_buf, in_cnt, in_dtype, out_buf,
                                              expected_out_cnt, out_dtype);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_wait_value_entry(iccl_sched* sched,
                                                                  const volatile uint64_t* ptr,
                                                                  uint64_t expected_value,
                                                                  iccl_condition condition)
{
    auto e = std::make_shared<wait_value_entry>(sched, ptr, expected_value, condition);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_function_entry(iccl_sched* sched,
                                                                iccl_sched_entry_function_t fn,
                                                                const void* ctx)
{
    auto e = std::make_shared<function_entry>(sched, fn, ctx);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_register_entry(iccl_sched* sched,
                                                                size_t size,
                                                                const iccl_buffer ptr,
                                                                atl_mr_t** mr)
{
    auto e = std::make_shared<register_entry>(sched, size, ptr, mr);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_deregister_entry(iccl_sched* sched,
                                                                  std::list<atl_mr_t*>& mr_list)
{
    auto e = std::make_shared<deregister_entry>(sched, mr_list);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_probe_entry(iccl_sched* sched,
                                                             size_t src,
                                                             size_t* cnt,
                                                             iccl_op_id_t op_id)
{
    std::shared_ptr<sched_entry> e = std::make_shared<probe_entry>(sched, global_data.comm->get_global_rank(src), cnt,
                                                                   op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_allreduce_entry(iccl_sched* sched,
                                                                 const iccl_buffer send_buf,
                                                                 iccl_buffer recv_buf,
                                                                 size_t cnt,
                                                                 iccl_datatype_internal_t dtype,
                                                                 iccl_reduction_t reduction_op)
{
    std::shared_ptr<sched_entry> e = std::make_shared<allreduce_entry>(sched, send_buf, recv_buf, cnt, dtype,
                                                                       reduction_op);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_allgatherv_entry(iccl_sched* sched,
                                                                  const iccl_buffer send_buf,
                                                                  size_t send_cnt,
                                                                  iccl_buffer recv_buf,
                                                                  size_t* recv_cnts,
                                                                  iccl_datatype_internal_t dtype)
{
    std::shared_ptr<sched_entry> e = std::make_shared<allgatherv_entry>(sched, send_buf, send_cnt, recv_buf,
                                                                        recv_cnts, dtype);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_bcast_entry(iccl_sched* sched,
                                                             iccl_buffer buf,
                                                             size_t cnt,
                                                             iccl_datatype_internal_t dtype,
                                                             size_t root)
{
    std::shared_ptr<sched_entry> e = std::make_shared<bcast_entry>(sched, buf, cnt, dtype, root);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_reduce_entry(iccl_sched *sched,
                                                              const iccl_buffer send_buf,
                                                              iccl_buffer recv_buf,
                                                              size_t cnt,
                                                              iccl_datatype_internal_t dtype,
                                                              iccl_reduction_t reduction,
                                                              size_t root)
{
    std::shared_ptr<sched_entry> e = std::make_shared<reduce_entry>(sched, send_buf, recv_buf,
                                                                    cnt, dtype, reduction, root);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_barrier_entry(iccl_sched *sched)
{
    std::shared_ptr<sched_entry> e = std::make_shared<barrier_entry>(sched);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_chain_call_entry(iccl_sched* sched,
                                                                  std::function<void(iccl_sched*)> sched_fill_function,
                                                                  const char* entry_name)
{
    std::shared_ptr<sched_entry> e = std::make_shared<chain_call_entry>(sched, sched_fill_function, entry_name);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_nop_entry(iccl_sched* sched)
{
    auto e = std::make_shared<nop_entry>(sched);
    sched->add_entry(e);
    return e;
}
