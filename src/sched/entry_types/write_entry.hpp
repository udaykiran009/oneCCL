#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"

class write_entry : public sched_entry
{
public:
    write_entry() = delete;
    write_entry(mlsl_sched* sched,
                const void* src_buf,
                atl_mr_t* src_mr,
                size_t cnt,
                mlsl_datatype_internal_t dtype,
                size_t dst,
                atl_mr_t* dst_mr,
                size_t dst_buf_off) :
        sched_entry(sched), src_buf(src_buf), src_mr(src_mr),
        cnt(cnt), dtype(dtype), dst(dst), dst_mr(dst_mr),
        dst_buf_off(dst_buf_off)
    {
        pfields.add_available(mlsl_sched_entry_field_src_mr);
        pfields.add_available(mlsl_sched_entry_field_dst_mr);
    }

    void start_derived()
    {
        MLSL_LOG(DEBUG, "WRITE entry dst %zu, req %p", dst, &req);
        MLSL_ASSERTP(src_buf && src_mr && dst_mr);

        if (!cnt)
        {
            status = mlsl_sched_entry_status_complete;
            return;
        }

        atl_status_t atl_status = atl_comm_write(sched->bin->comm_ctx, src_buf,
                                                 cnt * mlsl_datatype_get_size(dtype), src_mr,
                                                 (uint64_t)dst_mr->buf + dst_buf_off,
                                                 dst_mr->r_key, dst, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            status = mlsl_sched_entry_status_failed;
            MLSL_LOG(ERROR, "WRITE entry failed. atl_status: %d", atl_status);
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
            case mlsl_sched_entry_field_src_mr: return &src_mr;
            case mlsl_sched_entry_field_dst_mr: return &dst_mr;
            default: MLSL_ASSERTP(0);
        }
    }

    const char* name() const
    {
        return "WRITE";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, cnt %zu, src_buf %p, src_mr %p, dst %zu, "
                                     "dst_mr %p, dst_off %zu, comm %p, req %p\n",
                                     mlsl_datatype_get_name(dtype), cnt, src_buf, src_mr,
                                     dst, dst_mr, dst_buf_off, sched->coll_param.comm, &req);
        return dump_buf + bytes_written;
    }

private:
    const void *src_buf;
    atl_mr_t* src_mr;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    size_t dst;
    atl_mr_t* dst_mr;
    size_t dst_buf_off;
    atl_req_t req{};
};
