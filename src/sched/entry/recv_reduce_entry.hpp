#pragma once

#include "common/global/global.hpp"
#include "comp/comp.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched_queue.hpp"

#include <utility>

class recv_reduce_entry final: public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "RECV_REDUCE";
    }

    recv_reduce_entry() = delete;
    recv_reduce_entry(ccl_sched* sched,
                      ccl_buffer inout_buf,
                      size_t cnt,
                      size_t* out_cnt,
                      ccl_datatype_internal_t dtype,
                      ccl_reduction_t reduction_op,
                      size_t src,
                      ccl_buffer comm_buf,
                      ccl_op_id_t op_id = 0) :
        sched_entry(sched), inout_buf(inout_buf), in_cnt(cnt), out_cnt(out_cnt), dtype(dtype),
        op(reduction_op), src(src), comm_buf(comm_buf), op_id(op_id), fn(sched->coll_attr.reduction_fn)
    {
        CCL_ASSERT(op != ccl_reduction_custom || fn,
                    "custom reduction requires user provided callback");

        if (comm_buf.get_ptr() == nullptr || comm_buf == inout_buf)
        {
            size_t comm_buf_size = in_cnt * ccl_datatype_get_size(dtype);
            this->comm_buf.set(CCL_MALLOC(comm_buf_size, "recv_reduce.comm_buf"), comm_buf_size);
            own_comm_buff = true;
        }
    }

    void start_derived() override
    {
        atl_tag = global_data.atl_tag->create(sched->coll_param.comm->id(), src, sched->sched_id, op_id);
        size_t bytes = in_cnt * ccl_datatype_get_size(dtype);
        LOG_DEBUG("starting RECV in RECV_REDUCE entry, src ", src, ", tag ", atl_tag, ", req ", &req, ", bytes", bytes);

        atl_status_t atl_status = atl_comm_recv(sched->bin->get_comm_ctx(), comm_buf.get_ptr(bytes),
                                                bytes, src, atl_tag, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("RECV_REDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
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
            CCL_THROW("RECV_REDUCE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
        {
            LOG_DEBUG("completed RECV in RECV_REDUCE entry, req=", &req, ", starting REDUCE");
            size_t bytes = in_cnt * ccl_datatype_get_size(dtype);
            ccl_status_t comp_status = ccl_comp_reduce(comm_buf.get_ptr(bytes), in_cnt,
                                                         inout_buf.get_ptr(bytes),
                                                         out_cnt, dtype, op, fn);
            CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
            status = ccl_sched_entry_status_complete;
            LOG_DEBUG("completed REDUCE in RECV_REDUCE entry");
        }
    }

    const char* name() const override
    {
        return class_name();
    }

    ~recv_reduce_entry() override
    {
         if (own_comm_buff)
         {
             CCL_FREE(comm_buf.get_ptr());
         }
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", ccl_datatype_get_name(dtype),
                           ", inout_buf ", inout_buf,
                           ", in_cnt ", in_cnt,
                           ", out_cnt ", out_cnt,
                           ", op ", ccl_reduction_to_str(op),
                           ", red_fn  ", fn,
                           ", src ", src,
                           ", comm_buf ", comm_buf,
                           ", atl_tag ", atl_tag,
                           ", comm_id ", sched->coll_param.comm->id(),
                           ", req ", &req,
                           "\n");
    }

private:
    ccl_buffer inout_buf;
    size_t in_cnt;
    size_t* out_cnt;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t op;
    size_t src;
    ccl_buffer comm_buf;
    bool own_comm_buff = false;
    ccl_op_id_t op_id = 0;
    uint64_t atl_tag = 0;
    ccl_reduction_fn_t fn;
    atl_req_t req{};
};
