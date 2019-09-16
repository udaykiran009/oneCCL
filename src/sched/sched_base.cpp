#include "sched/sched_base.hpp"
#include "common/global/global.hpp"
#include "common/env/env.hpp"

void ccl_sched_base::set_coll_attr(const ccl_coll_attr& attr)
{
    coll_attr = attr;
}

void ccl_sched_base::update_coll_param(const ccl_coll_param& param)
{
#ifdef ENABLE_SYCL
    if (param.stream && (param.stream->get_type() == ccl_stream_sycl))
    {
        coll_param.sycl_buf = static_cast<ccl_sycl_buffer_t*>(param.buf);
        coll_param.sycl_send_buf = static_cast<ccl_sycl_buffer_t*>((void*)param.send_buf);
        coll_param.sycl_recv_buf = static_cast<ccl_sycl_buffer_t*>(param.recv_buf);
    }
    else
    {
#endif /* ENABLE_SYCL */
    coll_param.buf = param.buf;
    coll_param.send_buf = param.send_buf;
    coll_param.recv_buf = param.recv_buf;
#ifdef ENABLE_SYCL
    }
#endif /* ENABLE_SYCL */

    coll_param.recv_counts = param.recv_counts;

    if (coll_param.ctype == ccl_coll_sparse_allreduce)
    {
        coll_param.sparse_param.send_ind_buf = param.sparse_param.send_ind_buf;
        coll_param.sparse_param.send_val_buf = param.sparse_param.send_val_buf;
        coll_param.sparse_param.recv_ind_buf = param.sparse_param.recv_ind_buf;
        coll_param.sparse_param.recv_val_buf = param.sparse_param.recv_val_buf;
    }
}

void ccl_sched_base::update_coll_attr(const ccl_coll_attr& attr)
{
    if (env_data.priority_mode == ccl_priority_direct)
    {
        coll_attr.priority = attr.priority;
    }
}

size_t ccl_sched_base::get_priority() const
{
    size_t priority = 0;

    switch (env_data.priority_mode)
    {
        case ccl_priority_none:
            priority = 0;
            break;
        case ccl_priority_direct:
        case ccl_priority_lifo:
            priority = coll_attr.priority;
            break;
        default:
            CCL_FATAL("unexpected priority_mode ", env_data.priority_mode);
            break;
    }

    LOG_DEBUG("sched, ", this, ", priority ", priority);

    return priority;
}

ccl_buffer ccl_sched_base::alloc_buffer(size_t bytes)
{
    LOG_DEBUG("bytes ", bytes);
    CCL_THROW_IF_NOT(bytes > 0, "incorrect buffer size: ", bytes);

    ccl_buffer buffer = ccl_buffer(CCL_CALLOC(bytes, "sched_buffer"),
                                     bytes, 0, ccl_buffer_type::DIRECT);
    memory.buf_list.emplace_back(buffer, bytes);
    return buffer;
}

void ccl_sched_base::free_buffers()
{
    std::list<ccl_sched_buffer_handler>::iterator it;
    for (it = memory.buf_list.begin(); it != memory.buf_list.end(); it++)
    {
        LOG_DEBUG("free ", it->buffer.get_ptr());
        CCL_FREE(it->buffer.get_ptr());
    }
    memory.buf_list.clear();
}

void ccl_sched_base::alloc_buffers_for_sycl_copy()
{
#ifdef ENABLE_SYCL

    ccl_coll_param& param = coll_param;

    if (!param.stream || param.stream->get_type() != ccl_stream_sycl)
        return;

    LOG_DEBUG("alloc tmp buffers for D2H and H2D copies, coll_type ", ccl_coll_type_to_str(param.ctype));

    size_t idx, recv_count = 0;

    switch (param.ctype)
    {
        case ccl_coll_allgatherv:
            param.sycl_send_buf = static_cast<ccl_sycl_buffer_t*>((void*)param.send_buf);
            param.sycl_recv_buf = static_cast<ccl_sycl_buffer_t*>(param.recv_buf);
            param.send_buf = alloc_buffer(param.send_count * ccl_datatype_get_size(param.dtype)).get_ptr();
            for (idx = 0; idx < param.comm->size(); idx++)
                 recv_count += param.recv_counts[idx];
            param.recv_buf = alloc_buffer(recv_count * ccl_datatype_get_size(param.dtype)).get_ptr();
            break;
        case ccl_coll_allreduce:
            param.sycl_send_buf = static_cast<ccl_sycl_buffer_t*>((void*)param.send_buf);
            param.sycl_recv_buf = static_cast<ccl_sycl_buffer_t*>(param.recv_buf);
            param.send_buf = alloc_buffer(param.count * ccl_datatype_get_size(param.dtype)).get_ptr();
            param.recv_buf = alloc_buffer(param.count * ccl_datatype_get_size(param.dtype)).get_ptr();
            break;
        case ccl_coll_bcast:
            param.sycl_buf = static_cast<ccl_sycl_buffer_t*>(param.buf);
            param.buf = alloc_buffer(param.count * ccl_datatype_get_size(param.dtype)).get_ptr();
            break;
        case ccl_coll_reduce:
            param.sycl_send_buf = static_cast<ccl_sycl_buffer_t*>((void*)(param.send_buf));
            param.send_buf = alloc_buffer(param.count * ccl_datatype_get_size(param.dtype)).get_ptr();
            if (param.comm->rank() == param.root)
            {
                param.sycl_recv_buf = static_cast<ccl_sycl_buffer_t*>(param.recv_buf);
                param.recv_buf = alloc_buffer(param.count * ccl_datatype_get_size(param.dtype)).get_ptr();
            }
            else
            {
                param.recv_buf = nullptr;
            }
            break;
        case ccl_coll_sparse_allreduce:
            CCL_FATAL("SYCL stream is not supported for sparse_allreduce yet");
            CCL_ASSERT(0);
            break;
        default:
            break;
    }
#endif /* ENABLE_SYCL */
}

void ccl_sched_base::dump(std::ostream& out, const char *name) const
{
    ccl_logger::format(out, "\n-----------------", name, "---------------\n");
    ccl_logger::format(out,
                        "sched: ", this,
                        ", coll ", ccl_coll_type_to_str(coll_param.ctype),
                        ", comm_id ", std::dec, coll_param.comm->id(),
                        ", sched_id ", sched_id);
}

