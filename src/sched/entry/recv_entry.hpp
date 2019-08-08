#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

class recv_entry : public sched_entry
{
public:
    static constexpr const char* entry_class_name() noexcept
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
        pfields.add_available(ccl_sched_entry_field_buf);
        pfields.add_available(ccl_sched_entry_field_cnt);
    }

    void start_derived() override
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), src, sched->sched_id, op_id);
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        LOG_DEBUG("RECV entry src ", src, ", tag ", atl_tag, ", req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), buf.get_ptr(bytes),
                                                bytes, src, atl_tag, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
        {
            status = ccl_sched_entry_status_started;
        }
    }

    void update_derived() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("RECV entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            LOG_DEBUG("RECV entry done src, ", src);
            status = ccl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(ccl_sched_entry_field_id id) override
    {
        switch (id)
        {
            case ccl_sched_entry_field_buf: return &buf;
            case ccl_sched_entry_field_cnt: return &cnt;
            default: CCL_FATAL("unexpected id ", id);
        }
    }

    const char* name() const override
    {
        return entry_class_name();
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
