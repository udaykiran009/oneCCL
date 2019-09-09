#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/queue/queue.hpp"

class recv_entry : public sched_entry,
                   public postponed_fields<recv_entry,
                                           ccl_sched_entry_field_buf,
                                           ccl_sched_entry_field_cnt>
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "RECV";
    }

    recv_entry() = delete;
    recv_entry(ccl_sched* sched,
               ccl_buffer buf,
               size_t cnt,
               ccl_datatype_internal_t dtype,
               size_t src,
               ccl_op_id_t op_id = 0) :
        sched_entry(sched), buf(buf), cnt(cnt), dtype(dtype), src(src), op_id(op_id)
    {
    }

    ~recv_entry()
    {
        if (status == ccl_sched_entry_status_started)
        {
            size_t bytes = cnt * ccl_datatype_get_size(dtype);
            LOG_DEBUG("cancel RECV entry src ", src, ", req ", &req, ", bytes ", bytes);
            atl_comm_cancel(sched->bin->get_comm_ctx(), &req);
        }
    }

    void start() override
    {
        update_fields();

        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), src, sched->sched_id, op_id);
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        LOG_DEBUG("RECV entry src ", src, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), buf.get_ptr(bytes),
                                                bytes, src, atl_tag, &req);
        update_status(atl_status);
    }

    void update() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            LOG_DEBUG("RECV entry done, src ", src);
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override
    {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_buf> id)
    {
        return buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id)
    {
        return cnt;
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", ccl_datatype_get_name(dtype),
                           ", cnt ", cnt,
                           ", buf ", buf,
                           ", src ", src,
                           ", atl_tag ", atl_tag,
                           ", comm_id ", sched->coll_param.comm->id(),
                           ", req ", &req,
                           "\n");
    }

private:
    ccl_buffer buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    size_t src;
    ccl_op_id_t op_id = 0;
    uint64_t atl_tag = 0;
    atl_req_t req{};
};
