#include "sched/sched_base.hpp"
#include "common/global/global.hpp"
#include "common/env/env.hpp"


void ccl_sched_base::set_coll_attr(const ccl_coll_attr_t* attr,
                              std::string match_id)
{
    CCL_ASSERT(attr);
    coll_attr.prologue_fn = attr->prologue_fn;
    coll_attr.epilogue_fn = attr->epilogue_fn;
    coll_attr.reduction_fn = attr->reduction_fn;
    coll_attr.priority = attr->priority;
    coll_attr.synchronous = attr->synchronous;
    coll_attr.to_cache = attr->to_cache;
    coll_attr.match_id = std::move(match_id);
}

void ccl_sched_base::update_coll_param(ccl_coll_param& param)
{
    free_sycl_buffers();
    coll_param.buf = param.buf;
    coll_param.send_buf = param.send_buf;
    coll_param.recv_buf = param.recv_buf;
    coll_param.recv_counts = param.recv_counts;

    if (coll_param.ctype == ccl_coll_sparse_allreduce)
    {
        coll_param.sparse_param.send_ind_buf = param.sparse_param.send_ind_buf;
        coll_param.sparse_param.send_val_buf = param.sparse_param.send_val_buf;
        coll_param.sparse_param.recv_ind_buf = param.sparse_param.recv_ind_buf;
        coll_param.sparse_param.recv_val_buf = param.sparse_param.recv_val_buf;
    }
}

void ccl_sched_base::update_coll_attr(const ccl_coll_attr_t* attr)
{
    if (env_data.priority_mode == ccl_priority_direct)
    {
        coll_attr.priority = attr->priority;
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

void ccl_sched_base::free_sycl_buffers()
{
#ifdef ENABLE_SYCL
    switch(coll_param.ctype)
    {
        case ccl_coll_allgatherv:
            if (coll_param.stream && coll_param.stream->get_type() == ccl_stream_sycl)
            {
                delete[] (char*)coll_param.send_buf;
                delete[] (char*)coll_param.recv_buf;
            }
            break;
        case ccl_coll_allreduce:
            if (coll_param.stream && coll_param.stream->get_type() == ccl_stream_sycl)
            {
                delete[] (char*)coll_param.send_buf;
                delete[] (char*)coll_param.recv_buf;
            }
            break;
        case ccl_coll_bcast:
            if (coll_param.stream && coll_param.stream->get_type() == ccl_stream_sycl)
            {
                delete[] (char*)coll_param.buf;
            }
            break;
        case ccl_coll_reduce:
            if (coll_param.stream && coll_param.stream->get_type() == ccl_stream_sycl)
            {
                delete[] (char*)coll_param.send_buf;
                if (coll_param.comm->rank() == coll_param.root)
                {
                    delete[] (char*)coll_param.recv_buf;
                }
            }
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
                        ", comm_id ", coll_param.comm->id(),
                        ", sched_id ", sched_id);
}

