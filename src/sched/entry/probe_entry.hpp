#pragma once

#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

class probe_entry : public sched_entry
{
public:
    probe_entry() = delete;
    probe_entry(ccl_sched* sched,
                size_t source,
                size_t* count,
                ccl_op_id_t op_id) :
        sched_entry(sched), src(source), cnt(count), op_id(op_id)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), src, sched->sched_id, op_id);
        LOG_DEBUG("PROBE entry src ", src, ", tag ", atl_tag);
        atl_status_t atl_status = atl_comm_probe(sched->bin->get_comm_ctx(), src, atl_tag, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            status = ccl_sched_entry_status_failed;
            LOG_ERROR("PROBE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
        {
            status = ccl_sched_entry_status_started;
        }
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("SEND entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = ccl_sched_entry_status_complete;
            LOG_DEBUG("completed PROBE entry req=", &req);
            status = ccl_sched_entry_status_complete;
            *cnt = req.recv_len;
        }
    }

    const char* name() const
    {
        return "PROBE";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        ccl_logger::format(str,
                           "cnt ", *cnt,
                           ", src ", src,
                           ", comm ", sched->coll_param.comm,
                           ", atl_tag ", atl_tag,
                           ", req ", &req,
                           "\n");
    }

private:
    size_t src;
    size_t* cnt;
    ccl_op_id_t op_id;
    uint64_t atl_tag{};
    atl_req_t req{};
};
