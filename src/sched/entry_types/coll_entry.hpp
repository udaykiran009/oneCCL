#pragma once

#include "sched/entry_types/entry.hpp"

class coll_entry : public sched_entry
{
public:
    coll_entry() = delete;
    coll_entry(mlsl_sched* sched,
               mlsl_coll_type coll_type,
               const void* send_buf,
               void* recv_buf,
               size_t cnt,
               mlsl_datatype_internal_t dtype,
               mlsl_reduction_t reduction_op)
        : sched_entry(sched, true), ctype(coll_type),
          send_buf(send_buf), recv_buf(recv_buf), cnt(cnt),
          dtype(dtype), op(reduction_op), req(nullptr)
    {
        pfields.add_available(mlsl_sched_entry_field_send_buf);
        pfields.add_available(mlsl_sched_entry_field_recv_buf);
        pfields.add_available(mlsl_sched_entry_field_cnt);
        pfields.add_available(mlsl_sched_entry_field_dtype);
    }

    void start_derived()
    {
        create_schedule();
        status = mlsl_sched_entry_status_started;
    }

    void update_derived()
    {
        MLSL_THROW_IF_NOT(req, "empty request");
        if (req->is_completed())
        {
            MLSL_LOG(DEBUG, "COLL entry, completed sched");
            delete req->sched;
            status = mlsl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(mlsl_sched_entry_field_id id)
    {
        switch (id)
        {
            case mlsl_sched_entry_field_send_buf:
                return &send_buf;
            case mlsl_sched_entry_field_recv_buf:
                return &recv_buf;
            case mlsl_sched_entry_field_cnt:
                return &cnt;
            case mlsl_sched_entry_field_dtype:
                return &dtype;
            default:
                MLSL_FATAL("unexpected id %d", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "COLL";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf,
                                     "dt %s, coll_type %s, send_buf %p, recv_buf %p, cnt %zu, op %s, comm %p, req %p\n",
                                     mlsl_datatype_get_name(dtype), mlsl_coll_type_to_str(ctype),
                                     send_buf, recv_buf, cnt, mlsl_reduction_to_str(op),
                                     sched->coll_param.comm, req);
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
                mlsl_coll_param coll_param{};

                coll_param.ctype = mlsl_coll_allreduce;
                coll_param.send_buf = send_buf;
                coll_param.recv_buf = recv_buf;
                coll_param.count = cnt;
                coll_param.dtype = dtype;
                coll_param.reduction = op;
                coll_param.comm = sched->coll_param.comm;

                coll_sched = new mlsl_sched(coll_param);

                coll_sched->coll_attr.reduction_fn = sched->coll_attr.reduction_fn;
                auto result = mlsl_coll_build_allreduce(coll_sched,
                                                        coll_sched->coll_param.send_buf,
                                                        coll_sched->coll_param.recv_buf,
                                                        coll_sched->coll_param.count,
                                                        coll_sched->coll_param.dtype,
                                                        coll_sched->coll_param.reduction);

                MLSL_ASSERT_FMT(result == mlsl_status_success, "bad result %d", result);

                break;
            }
            case mlsl_coll_allgatherv:
            default:
                MLSL_FATAL("not supported type %d", ctype);
                break;
        }
        if (coll_sched)
        {
            MLSL_LOG(DEBUG, "starting COLL entry");
            req = sched->start_subsched(coll_sched);
            MLSL_LOG(DEBUG, "COLL entry: sched %p, req %p", coll_sched, req);
            // TODO: insert into per-worker sched cache
        }
    }

    mlsl_coll_type ctype;
    const void* send_buf;
    void* recv_buf;
    size_t cnt;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t op;
    mlsl_request* req;
};
