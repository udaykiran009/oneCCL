#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"

class recv_entry : public sched_entry
{
public:
    recv_entry() = delete;
    recv_entry(mlsl_sched* sched,
               void* buf,
               size_t cnt,
               mlsl_datatype_internal_t dtype,
               size_t src) :
        sched_entry(sched), buf(buf), cnt(cnt), dtype(dtype), src(src)
    {
        pfields.add_available(mlsl_sched_entry_field_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
    }

    void start_derived()
    {
        auto atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->id(), sched->sched_id, src);
        size_t bytes = cnt * mlsl_datatype_get_size(dtype);
        MLSL_LOG(DEBUG, "RECV entry src %zu, tag %lu, req %p, bytes %zu", src, atl_tag, &req, bytes);
        atl_status_t atl_status = atl_comm_recv(sched->bin->comm_ctx, buf,
                                                bytes, src, atl_tag, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("RECV entry failed. atl_status: %d", atl_status);
        }
        else
            status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_comm_check(sched->bin->comm_ctx, &req_status, &req);
        if (req_status)
            status = mlsl_sched_entry_status_complete;
    }
    
    void* get_field_ptr(mlsl_sched_entry_field_id id)
    {
        switch (id)
        {
            case mlsl_sched_entry_field_buf: return &buf;
            case mlsl_sched_entry_field_cnt: return &cnt;
            default: MLSL_FATAL("unexpected id %d", id);
        }
    }

    const char* name() const
    {
        return "RECV";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, cnt %zu, buf %p, src %zu, comm_id %hu, req %p\n",
                                     mlsl_datatype_get_name(dtype), cnt, buf, src, sched->coll_param.comm->id(), &req);
        return dump_buf + bytes_written;
    }

private:
    void* buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    size_t src;
    atl_req_t req{};
};
