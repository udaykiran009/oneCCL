#pragma once

#include "sched/entry/entry.hpp"
#include "common/request/request.hpp"

class coll_entry : public sched_entry
{
public:
    coll_entry() = delete;
    coll_entry(ccl_sched* sched,
               ccl_coll_type coll_type,
               const ccl_buffer send_buf,
               ccl_buffer recv_buf,
               size_t cnt,
               ccl_datatype_internal_t dtype,
               ccl_reduction_t reduction_op,
               size_t root = 0)
        : sched_entry(sched, true), ctype(coll_type),
          send_buf(send_buf), recv_buf(recv_buf), cnt(cnt),
          dtype(dtype), op(reduction_op), req(nullptr), root(root)
    {
        LOG_DEBUG("creating ", name(), " entry");
        pfields.add_available(ccl_sched_entry_field_buf);
        pfields.add_available(ccl_sched_entry_field_send_buf);
        pfields.add_available(ccl_sched_entry_field_recv_buf);
        pfields.add_available(ccl_sched_entry_field_cnt);
        pfields.add_available(ccl_sched_entry_field_dtype);
    }

    void start_derived()
    {
        create_schedule();
        status = ccl_sched_entry_status_started;
    }

    void update_derived()
    {
        CCL_THROW_IF_NOT(req, "empty request");
        if (req->is_completed())
        {
            LOG_DEBUG("COLL entry, completed sched");
            delete req->sched;
            status = ccl_sched_entry_status_complete;
        }
    }

    void* get_field_ptr(ccl_sched_entry_field_id id)
    {
        switch (id)
        {
            case ccl_sched_entry_field_buf:
                return &recv_buf;
            case ccl_sched_entry_field_send_buf:
                return &send_buf;
            case ccl_sched_entry_field_recv_buf:
                return &recv_buf;
            case ccl_sched_entry_field_cnt:
                return &cnt;
            case ccl_sched_entry_field_dtype:
                return &dtype;
            default:
                CCL_FATAL("unexpected id ", id);
        }
        return nullptr;
    }

    const char* name() const
    {
        return "COLL";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        ccl_logger::format(str,
                            "dt ", ccl_datatype_get_name(dtype),
                            ", coll_type ", ccl_coll_type_to_str(ctype),
                            ", send_buf ", send_buf,
                            ", recv_buf ", recv_buf,
                            ", cnt ", cnt,
                            ", op ", ccl_reduction_to_str(op),
                            ", comm ", sched->coll_param.comm,
                            ", req ", req,
                            "\n");
    }

private:

    void create_schedule()
    {
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        ccl_sched* coll_sched = nullptr;
        switch (ctype)
        {
            case ccl_coll_barrier:
            case ccl_coll_bcast:
            {
                ccl_coll_param coll_param{};
                coll_param.ctype = ccl_coll_bcast;
                coll_param.buf = recv_buf.get_ptr(bytes);
                coll_param.count = cnt;
                coll_param.dtype = dtype;
                coll_param.root = root;
                coll_param.comm = sched->coll_param.comm;
                coll_sched = new ccl_sched(coll_param);

                auto result = ccl_coll_build_bcast(coll_sched,
                                                    recv_buf,
                                                    coll_sched->coll_param.count,
                                                    coll_sched->coll_param.dtype,
                                                    coll_sched->coll_param.root);

                CCL_ASSERT(result == ccl_status_success, "bad result ", result);

            }
            case ccl_coll_reduce:
                break;
            case ccl_coll_allreduce:
            {
                ccl_coll_param coll_param{};
                coll_param.ctype = ccl_coll_allreduce;
                coll_param.send_buf = send_buf.get_ptr(bytes);
                coll_param.recv_buf = recv_buf.get_ptr(bytes);
                coll_param.count = cnt;
                coll_param.dtype = dtype;
                coll_param.reduction = op;
                coll_param.comm = sched->coll_param.comm;
                coll_sched = new ccl_sched(coll_param);
                coll_sched->coll_attr.reduction_fn = sched->coll_attr.reduction_fn;

                auto result = ccl_coll_build_allreduce(coll_sched,
                                                        send_buf,
                                                        recv_buf,
                                                        coll_sched->coll_param.count,
                                                        coll_sched->coll_param.dtype,
                                                        coll_sched->coll_param.reduction);

                CCL_ASSERT(result == ccl_status_success, "bad result ", result);

                break;
            }
            case ccl_coll_allgatherv:
            default:
                CCL_FATAL("not supported type ", ctype);
                break;
        }

        if (coll_sched)
        {
            LOG_DEBUG("starting COLL entry");
            req = sched->start_subsched(coll_sched);
            LOG_DEBUG("COLL entry: sched ", coll_sched, ", req ", req);
            // TODO: insert into per-worker sched cache
        }
    }

    ccl_coll_type ctype;
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t op;
    ccl_request* req;
    //for bcast
    size_t root;
};
