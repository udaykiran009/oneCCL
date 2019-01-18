#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"
#include "comp/comp.hpp"

#include <utility>

//todo: inherit/compose recv & reduce

class recv_reduce_entry : public sched_entry
{
public:
    recv_reduce_entry() = delete;
    recv_reduce_entry(mlsl_sched* schedule,
                      void* inout_buffer,
                      size_t count,
                      size_t* out_count,
                      mlsl_datatype_internal_t data_type,
                      mlsl_reduction_t reduction_op,
                      mlsl_reduction_fn_t reduction_fn,
                      size_t source,
                      void* communication_buf,
                      const mlsl_comm* communicator) :
        sched_entry(schedule), inout_buf(inout_buffer), in_cnt(count), out_cnt(out_count), dtype(data_type),
        op(reduction_op), fn(reduction_fn), src(source), comm_buf(communication_buf), comm(communicator)
    {
        MLSL_LOG(DEBUG, "op %d fn %p", op, fn);
        MLSL_ASSERT_FMT(op != mlsl_reduction_custom || fn, "Custom reduction requires user provided callback");
        if (comm_buf == nullptr || comm_buf == inout_buf)
        {
            comm_buf = MLSL_MALLOC(in_cnt * mlsl_datatype_get_size(dtype), "recv_reduce.comm_buf");
            allocated_comm_buff = true;
        }
    }

    recv_reduce_entry(const recv_reduce_entry& other) :
        inout_buf(other.inout_buf), in_cnt(other.in_cnt), out_cnt(other.out_cnt), dtype(other.dtype),
        op(other.op), fn(other.fn), src(other.src), comm(other.comm), allocated_comm_buff(other.allocated_comm_buff)
    {
        if (allocated_comm_buff)
        {
            comm_buf = MLSL_MALLOC(in_cnt * mlsl_datatype_get_size(dtype), "recv_reduce.comm_buf");
        }
    }

    recv_reduce_entry& operator=(const recv_reduce_entry& other)
    {
        if (this != &other)
        {
            //copy and swap
            recv_reduce_entry(other).swap(*this);
        }

        return *this;
    }

    void execute()
    {
        if (status == mlsl_sched_entry_status_not_started)
        {
            auto atl_tag = mlsl_create_atl_tag(sched->coll_param.comm->comm_id, sched->sched_id, src);
            MLSL_LOG(DEBUG, "starting RECV in RECV_REDUCE entry, src %zu, tag %lu, req %p", src, atl_tag, &req);
            atl_status_t atl_status = atl_comm_recv(sched->bin->comm_ctx, comm_buf,
                                                    in_cnt * mlsl_datatype_get_size(dtype),
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
                MLSL_LOG(DEBUG, "completed RECV in RECV_REDUCE entry, req=%p, starting REDUCE", &req);
                mlsl_status_t comp_status = mlsl_comp_reduce(comm_buf, in_cnt, inout_buf, out_cnt, dtype, op, fn);
                MLSL_ASSERT(comp_status == mlsl_status_success);
                status = mlsl_sched_entry_status_complete;
                MLSL_LOG(DEBUG, "completed REDUCE in RECV_REDUCE entry");
            }
        }
    }

    void adjust(size_t partition_idx,
                size_t partition_count)
    {
        size_t adjust_count, adjust_offset;
        get_count_and_offset(in_cnt, dtype, partition_idx, partition_count, adjust_count, adjust_offset);
        in_cnt = adjust_count;
        adjust_ptr(inout_buf, adjust_offset);
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

    std::shared_ptr<sched_entry> clone() const
    {
        //call defined copy ctor
        return std::make_shared<recv_reduce_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "dt %s, inout_buf %p, in_cnt %zu, out_cnt %p, op %s, red_fn %p, "
                                               "src %zu, comm_buf %p, comm %p, req %p\n",
                                     mlsl_datatype_get_name(dtype), inout_buf, in_cnt, out_cnt,
                                     mlsl_reduction_to_str(op), fn, src, comm_buf, comm, &req);
        return dump_buf + bytes_written;
    }

private:

    void swap(recv_reduce_entry& other)
    {
        std::swap(inout_buf, other.inout_buf);
        std::swap(in_cnt, other.in_cnt);
        std::swap(out_cnt, other.out_cnt);
        std::swap(dtype, other.dtype);
        std::swap(op, other.op);
        std::swap(fn, other.fn);
        std::swap(src, other.src);
        std::swap(comm_buf, other.comm_buf);
        std::swap(comm, other.comm);
        std::swap(req, other.req);
    }

    void* inout_buf;
    size_t in_cnt;
    size_t* out_cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_reduction_fn_t fn;
    size_t src;
    void* comm_buf;
    const mlsl_comm* comm;
    atl_req_t req{};

    //todo: rework
    bool allocated_comm_buff = false;
};
