#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched.hpp"

class collective_entry : public sched_entry
{
public:
    collective_entry() = delete;
    collective_entry(mlsl_sched* schedule,
                     mlsl_coll_type coll_type,
                     const void* send_buffer,
                     void* recv_buffer,
                     size_t count,
                     mlsl_datatype_internal_t data_type,
                     mlsl_reduction_t reduction_op,
                     mlsl_comm* communicator)
        : sched_entry(schedule, true), ctype(coll_type), send_buf(send_buffer), recv_buf(recv_buffer), cnt(count),
          dtype(data_type), op(reduction_op), comm(communicator), req(nullptr)
    {}

    void execute()
    {
        if (status == mlsl_sched_entry_status_started)
        {
            MLSL_ASSERTP(req);
            if (mlsl_request_is_complete(req))
            {
                MLSL_LOG(DEBUG, "COLLECTIVE entry, completed coll_sched");
                delete req->sched;
                status = mlsl_sched_entry_status_complete;
            }
            return;
        }

        create_schedule();
    }

    const char* name() const
    {
        return "COLLECTIVE";
    }

    std::shared_ptr<sched_entry> clone() const
    {
        //todo: we can eithir make a full copy or a new object with the same inputs
        return std::make_shared<collective_entry>(*this);
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "dt %s, coll_type %s, send_buf %p, recv_buf %p, count %zu, op %s, comm %p, req %p\n",
                                     mlsl_datatype_get_name(dtype), mlsl_coll_type_to_str(ctype),
                                     send_buf, recv_buf, cnt, mlsl_reduction_to_str(op),
                                     comm, &req);
        return dump_buf + bytes_written;
    }

private:

    void create_schedule()
    {
        mlsl_sched* coll_sched = nullptr;
        switch (ctype)
        {
            case mlsl_coll_barrier:
            case mlsl_coll_bcast:
            case mlsl_coll_reduce:
                break;
            case mlsl_coll_allreduce:
            {
                auto elem_sbuf = send_buf == MLSL_POSTPONED_ADDR ? sched->postponed_fields.buf : send_buf;
                auto elem_rbuf = recv_buf == MLSL_POSTPONED_ADDR ? sched->postponed_fields.buf : recv_buf;
                auto elem_count = cnt == MLSL_POSTPONED_COUNT ? sched->postponed_fields.count : cnt;
                auto elem_dtype = dtype == MLSL_POSTPONED_DTYPE ? &sched->postponed_fields.dtype : dtype;

                mlsl_status_t result = mlsl_sched_allreduce(elem_sbuf,
                                                            elem_rbuf,
                                                            elem_count,
                                                            elem_dtype,
                                                            op,
                                                            sched->coll_param.comm,
                                                            &coll_sched);
                MLSL_ASSERT(result == mlsl_status_success);

                coll_sched->coll_attr.reduction_fn = sched->coll_attr.reduction_fn;
                result = mlsl_coll_build_allreduce(coll_sched,
                                                   coll_sched->coll_param.send_buf,
                                                   coll_sched->coll_param.recv_buf,
                                                   coll_sched->coll_param.count,
                                                   coll_sched->coll_param.dtype,
                                                   coll_sched->coll_param.reduction);

                MLSL_ASSERT(result == mlsl_status_success);

                break;
            }
            case mlsl_coll_allgatherv:
            case mlsl_coll_custom:
            default:
                /* only allreduce for now */
                MLSL_ASSERTP(0);
                break;
        }
        if (coll_sched)
        {
            MLSL_LOG(DEBUG, "starting COLLECTIVE entry");
            mlsl_request* coll_req;
            coll_sched->sched_id = sched->sched_id;
            mlsl_request_create(&coll_req);
            coll_req->completion_counter = 1;
            coll_req->sched = coll_sched;
            coll_sched->req = coll_req;
            req = coll_req;
            mlsl_sched_reset(coll_sched);
            mlsl_sched_dump(coll_sched, "coll_sched");
            sched->bin->queue->add(coll_sched, mlsl_sched_get_priority(sched));
            MLSL_LOG(DEBUG, "COLLECTIVE entry: queue %p, sched %p, req %p",
                     sched->bin->queue, coll_sched, coll_req);
            // TODO: insert into per-worker sched cache
        }
        status = mlsl_sched_entry_status_started;
    }

    mlsl_coll_type ctype;
    const void* send_buf;
    void* recv_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_comm* comm;
    mlsl_request* req;
};
