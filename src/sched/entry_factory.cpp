#include "sched/sched.hpp"
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
#include "sched/entry_types/wait_value_entry.hpp"
#include "sched/entry_types/function_entry.hpp"
#include "sched/entry_types/probe_entry.hpp"
#include "sched/entry_types/register_entry.hpp"
#include "sched/entry_types/deregister_entry.hpp"
#include "sched/entry_types/chain_call_entry.hpp"
#include "sched/entry_types/nop_entry.hpp"

std::shared_ptr<sched_entry> entry_factory::make_send_entry(mlsl_sched* sched,
                                                            const void* buf,
                                                            size_t cnt,
                                                            mlsl_datatype_internal_t dtype,
                                                            size_t dst,
                                                            mlsl_op_id_t op_id)
{
    auto e = std::make_shared<send_entry>(sched, buf, cnt, dtype,
                                          sched->coll_param.comm->get_global_rank(dst), global_data.comm->rank(),
                                          op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_entry(mlsl_sched* sched,
                                                            void* buf,
                                                            size_t cnt,
                                                            mlsl_datatype_internal_t dtype,
                                                            size_t src,
                                                            mlsl_op_id_t op_id)
{
    auto e = std::make_shared<recv_entry>(sched, buf, cnt, dtype, sched->coll_param.comm->get_global_rank(src), op_id);
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
    auto e = std::make_shared<write_entry>(sched, src_buf, src_mr, cnt, dtype,
                                           dst, dst_mr, dst_buf_offset);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_reduce_entry(mlsl_sched* sched,
                                                              const void* in_buf,
                                                              size_t in_cnt,
                                                              void* inout_buf,
                                                              size_t* out_cnt,
                                                              mlsl_datatype_internal_t dtype,
                                                              mlsl_reduction_t reduction_op)
{
    auto e = std::make_shared<reduce_entry>(sched, in_buf, in_cnt, inout_buf,
                                            out_cnt, dtype, reduction_op);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_recv_reduce_entry(mlsl_sched* sched,
                                                                   void* inout_buf,
                                                                   size_t in_cnt,
                                                                   size_t* out_cnt,
                                                                   mlsl_datatype_internal_t dtype,
                                                                   mlsl_reduction_t reduction_op,
                                                                   size_t src,
                                                                   void* comm_buf,
                                                                   mlsl_op_id_t op_id)
{
    auto e = std::make_shared<recv_reduce_entry>(sched, inout_buf, in_cnt, out_cnt, dtype, reduction_op,
                                                 sched->coll_param.comm->get_global_rank(src), comm_buf, op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_copy_entry(mlsl_sched* sched,
                                                            const void* in_buf,
                                                            void* out_buf,
                                                            size_t cnt,
                                                            mlsl_datatype_internal_t dtype)
{
    auto e = std::make_shared<copy_entry>(sched, in_buf, out_buf, cnt, dtype);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_sync_entry(mlsl_sched* sched,
                                                            std::shared_ptr<sync_object> sync_obj)
{
    auto e = std::make_shared<sync_entry>(sched, sync_obj);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_coll_entry(mlsl_sched* sched,
                                                            mlsl_coll_type coll_type,
                                                            const void* send_buf,
                                                            void* recv_buf,
                                                            size_t cnt,
                                                            mlsl_datatype_internal_t dtype,
                                                            mlsl_reduction_t reduction_op,
                                                            size_t root)
{
    auto e = std::make_shared<coll_entry>(sched, coll_type, send_buf, recv_buf,
                                          cnt, dtype, reduction_op, root);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_prologue_entry(mlsl_sched* sched,
                                                                mlsl_prologue_fn_t fn,
                                                                const void* in_buf,
                                                                size_t in_cnt,
                                                                mlsl_datatype_internal_t in_dtype,
                                                                void** out_buf,
                                                                size_t* out_cnt,
                                                                mlsl_datatype_t* out_dtype,
                                                                size_t* out_dtype_size)
{
    auto e = std::make_shared<prologue_entry>(sched, fn, in_buf, in_cnt, in_dtype,
                                              out_buf, out_cnt, out_dtype, out_dtype_size);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_epilogue_entry(mlsl_sched* sched,
                                                                mlsl_epilogue_fn_t fn,
                                                                const void* in_buf,
                                                                size_t in_cnt,
                                                                mlsl_datatype_internal_t in_dtype,
                                                                void* out_buf,
                                                                size_t expected_out_cnt,
                                                                mlsl_datatype_internal_t out_dtype)
{
    auto e = std::make_shared<epilogue_entry>(sched, fn, in_buf, in_cnt, in_dtype, out_buf,
                                              expected_out_cnt, out_dtype);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_wait_value_entry(mlsl_sched* sched,
                                                                  const volatile uint64_t* ptr,
                                                                  uint64_t expected_value,
                                                                  mlsl_condition condition)
{
    auto e = std::make_shared<wait_value_entry>(sched, ptr, expected_value, condition);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_function_entry(mlsl_sched* sched,
                                                                mlsl_sched_entry_function_t fn,
                                                                const void* ctx)
{
    auto e = std::make_shared<function_entry>(sched, fn, ctx);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_register_entry(mlsl_sched* sched,
                                                                size_t size,
                                                                void* ptr,
                                                                atl_mr_t** mr)
{
    auto e = std::make_shared<register_entry>(sched, size, ptr, mr);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_deregister_entry(mlsl_sched* sched,
                                                                  std::list<atl_mr_t*>& mr_list)
{
    auto e = std::make_shared<deregister_entry>(sched, mr_list);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_probe_entry(mlsl_sched* sched,
                                                             size_t src,
                                                             size_t* cnt,
                                                             mlsl_op_id_t op_id)
{
    std::shared_ptr<sched_entry> e = std::make_shared<probe_entry>(sched, global_data.comm->get_global_rank(src), cnt,
                                                                   op_id);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_chain_call_entry(mlsl_sched* sched,
                                                                  std::function<void(mlsl_sched*)> sched_fill_function,
                                                                  const char* entry_name)
{
    std::shared_ptr<sched_entry> e = std::make_shared<chain_call_entry>(sched, sched_fill_function, entry_name);
    sched->add_entry(e);
    return e;
}

std::shared_ptr<sched_entry> entry_factory::make_nop_entry(mlsl_sched* sched)
{
    auto e = std::make_shared<nop_entry>(sched);
    sched->add_entry(e);
    return e;
}
