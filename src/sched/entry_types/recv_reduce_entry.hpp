#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"
#include "comp/comp.hpp"

#include <utility>

//todo: inherit/compose recv & reduce

class recv_reduce_entry : public sched_entry
{
public:
    recv_reduce_entry() = delete;
    recv_reduce_entry(mlsl_sched* sched,
                      void* inout_buf,
                      size_t cnt,
                      size_t* out_cnt,
                      mlsl_datatype_internal_t dtype,
                      mlsl_reduction_t reduction_op,
                      size_t src,
                      void* comm_buf) :
        sched_entry(sched), inout_buf(inout_buf), in_cnt(cnt), out_cnt(out_cnt), dtype(dtype),
        op(reduction_op), src(src), comm_buf(comm_buf), fn(sched->coll_attr.reduction_fn)
    {
        LOG_DEBUG("creating ", name(), " entry");
        MLSL_ASSERT(op != mlsl_reduction_custom || fn,
            "custom reduction requires user provided callback");
        if (comm_buf == nullptr || comm_buf == inout_buf)
        {
            this->comm_buf = MLSL_MALLOC(in_cnt * mlsl_datatype_get_size(dtype), "recv_reduce.comm_buf");
            allocated_comm_buff = true;
        }
    }

    void start_derived()
    {
        auto atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->id(), sched->sched_id, src);
        size_t bytes = in_cnt * mlsl_datatype_get_size(dtype);
        LOG_DEBUG("starting RECV in RECV_REDUCE entry, src ", src, ", tag ", std::setbase(16), atl_tag,
            ", req ", &req, ", bytes", bytes);
        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), comm_buf,
                                                bytes, src, atl_tag, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("RECV_REDUCE entry failed. atl_status: ", atl_status);
        }
        else
            status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("RECV_REDUCE entry failed. atl_status: ", atl_status);
        }

        if (req_status)
        {
            LOG_DEBUG("completed RECV in RECV_REDUCE entry, req=", &req, ", starting REDUCE");
            mlsl_status_t comp_status = mlsl_comp_reduce(comm_buf, in_cnt, inout_buf, out_cnt,
                                                         dtype, op, fn);
            MLSL_ASSERT(comp_status == mlsl_status_success, "bad status ", comp_status);
            status = mlsl_sched_entry_status_complete;
            LOG_DEBUG("completed REDUCE in RECV_REDUCE entry");
        }
    }

    const char* name() const
    {
        return "RECV_REDUCE";
    }

    ~recv_reduce_entry()
    {
        if (allocated_comm_buff)
        {
            MLSL_FREE(comm_buf);
        }
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "dt ", mlsl_datatype_get_name(dtype),
                            ", inout_buf ", inout_buf,
                            ", in_cnt ", in_cnt,
                            ", out_cnt ", out_cnt,
                            ", op ", mlsl_reduction_to_str(op),
                            ", red_fn  ", fn,
                            ", src ", src,
                            ", comm_buf ", comm_buf,
                            ", comm_id ", sched->coll_param.comm->id(),
                            ", req ", &req,
                            "\n");
    }

private:
    void* inout_buf;
    size_t in_cnt;
    size_t* out_cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    size_t src;
    void* comm_buf;
    atl_req_t req{};
    bool allocated_comm_buff = false;
    mlsl_reduction_fn_t fn;
};
