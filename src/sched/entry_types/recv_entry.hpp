#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class recv_entry : public sched_entry
{
public:
    recv_entry() = delete;
    recv_entry(mlsl_sched* schedule,
               void* buffer,
               size_t count,
               mlsl_datatype_internal_t data_type,
               size_t source,
               const mlsl_comm* communicator) :
        sched_entry(schedule), buf(buffer), cnt(count), dtype(data_type), src(source), comm(communicator)
    {}

    void execute()
    {
        if (status == mlsl_sched_entry_status_not_started)
        {
            auto atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->comm_id, sched->sched_id, src);
            MLSL_LOG(DEBUG, "RECV entry src %zu, tag %lu, req %p", src, atl_tag, &req);

            atl_status_t atl_status = atl_comm_recv(sched->bin->comm_ctx, buf, cnt * mlsl_datatype_get_size(dtype),
                                                    src, atl_tag, &req);

            if (unlikely(atl_status != atl_status_success))
            {
                status = mlsl_sched_entry_status_failed;
                MLSL_LOG(ERROR, "RECV entry failed. atl_status: %d", atl_status);
            }
            else
            {
                status = mlsl_sched_entry_status_started;
            }
        }
        else if (status == mlsl_sched_entry_status_started)
        {
            int req_status;

            atl_comm_check(sched->bin->comm_ctx, &req_status, &req);
            if (req_status)
            {
                MLSL_LOG(DEBUG, "completed RECV entry req=%p", &req);
                status = mlsl_sched_entry_status_complete;
            }
        }
    }

    void adjust(size_t partition_idx,
                size_t partition_count)
    {
        size_t adjust_count, adjust_offset;
        get_count_and_offset(cnt, dtype, partition_idx, partition_count, adjust_count, adjust_offset);
        cnt = adjust_count;
        adjust_ptr(buf, adjust_offset);
    }

    const char* name() const
    {
        return "RECV";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //full member-wise copy
        return std::make_shared<recv_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, cnt %zu, buf %p, src %zu, comm %p, req %p\n",
                                     mlsl_datatype_get_name(dtype), cnt, buf, src, comm, &req);
        return dump_buf + bytes_written;
    }

private:
    void* buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    size_t src;
    const mlsl_comm* comm;
    atl_req_t req{};
};
