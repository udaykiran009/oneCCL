#pragma once

#include "sched/entry/coll/base_coll_entry.hpp"

class bcast_entry : public base_coll_entry
{
public:
    bcast_entry() = delete;
    bcast_entry(ccl_sched* sched,
                ccl_buffer buf,
                size_t cnt,
                ccl_datatype_internal_t dtype,
                size_t root) :
        base_coll_entry(sched), buf(buf),
        cnt(cnt), root(root), dtype(dtype)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        LOG_DEBUG("BCAST entry req ", &req, ", bytes ", bytes);

        atl_status_t atl_status = atl_comm_bcast(sched->bin->get_comm_ctx(), buf.get_ptr(bytes),
                                                 bytes, root, &req);
        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("BCAST entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("BCAST entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const
    {
        return "BCAST";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        ccl_logger::format(str,
                            "dt ", ccl_datatype_get_name(dtype),
                            ", cnt ", cnt,
                            ", root ", root,
                            ", buf ", buf,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ",&req,
                            "\n");
    }

private:
    ccl_buffer buf;
    size_t cnt;
    size_t root;
    ccl_datatype_internal_t dtype;
    atl_req_t req{};
};
