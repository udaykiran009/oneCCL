#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class probe_entry : public sched_entry
{
public:
    probe_entry() = delete;
    probe_entry(mlsl_sched* sched,
               size_t source,
               size_t* count) :
        sched_entry(sched), src(source), cnt(count)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        auto atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->id(), sched->sched_id, src);
        LOG_DEBUG("PROBE entry src ", src, ", tag ", std::setbase(16), atl_tag);
        atl_status_t atl_status = atl_comm_probe(sched->bin->get_comm_ctx(), src, atl_tag, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            status = mlsl_sched_entry_status_failed;
            LOG_ERROR("PROBE entry failed. atl_status: ", atl_status);
        }
        else
            status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);
        if (req_status)
        {
            status = mlsl_sched_entry_status_complete;
            LOG_DEBUG("completed PROBE entry req=", &req);
            status = mlsl_sched_entry_status_complete;
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
        mlsl_logger::format(str,
                            "cnt ", *cnt,
                            ", src ", src,
                            ", comm ", sched->coll_param.comm,
                            ", req ", &req,
                            "\n");
    }

private:
    size_t src;
    size_t* cnt;   
    atl_req_t req{};
};
