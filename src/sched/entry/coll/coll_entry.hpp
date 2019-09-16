#pragma once

#include "sched/entry/entry.hpp"
#include "common/request/request.hpp"
#include "sched/extra_sched.hpp"

class coll_entry : public sched_entry,
                   public postponed_fields<coll_entry,
                                           ccl_sched_entry_field_buf,
                                           ccl_sched_entry_field_send_buf,
                                           ccl_sched_entry_field_recv_buf,
                                           ccl_sched_entry_field_cnt,
                                           ccl_sched_entry_field_dtype>
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "COLL";
    }

    coll_entry() = delete;
    coll_entry(ccl_sched* sched,
               ccl_coll_type coll_type,
               const ccl_buffer send_buf,
               ccl_buffer recv_buf,
               size_t cnt,
               ccl_datatype_internal_t dtype,
               ccl_reduction_t reduction_op,
               size_t root = 0)
        : sched_entry(sched), ctype(coll_type),
          send_buf(send_buf), recv_buf(recv_buf), cnt(cnt),
          dtype(dtype), op(reduction_op), coll_sched(), root(root)
    {
    }

    void start() override
    {
        update_fields();

        create_schedule();
        status = ccl_sched_entry_status_started;
    }

    void update() override
    {
        CCL_THROW_IF_NOT(coll_sched, "empty request");
        if (coll_sched->is_completed())
        {
            LOG_DEBUG("COLL entry, completed sched: ", coll_sched.get());
            coll_sched.reset();
            status = ccl_sched_entry_status_complete;
        }
    }

    bool is_strict_order_satisfied() override
    {
        return is_completed();
    }

    const char* name() const override
    {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_buf> id)
    {
        return recv_buf;
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_send_buf> id)
    {
        return send_buf;
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_recv_buf> id)
    {
        return recv_buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id)
    {
        return cnt;
    }

    ccl_datatype_internal_t& get_field_ref(field_id_t<ccl_sched_entry_field_dtype> id)
    {
        return dtype;
    }
protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                            "dt ", ccl_datatype_get_name(dtype),
                            ", coll_type ", ccl_coll_type_to_str(ctype),
                            ", send_buf ", send_buf,
                            ", recv_buf ", recv_buf,
                            ", cnt ", cnt,
                            ", op ", ccl_reduction_to_str(op),
                            ", comm ", sched->coll_param.comm,
                            ", coll sched ", coll_sched.get(),
                            "\n");
    }

private:

    void create_schedule()
    {
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        switch (ctype)
        {
            case ccl_coll_barrier:
                break;
            case ccl_coll_bcast:
            {
                ccl_coll_param coll_param{};
                coll_param.ctype = ccl_coll_bcast;
                coll_param.buf = recv_buf.get_ptr(bytes);
                coll_param.count = cnt;
                coll_param.dtype = dtype;
                coll_param.root = root;
                coll_param.comm = sched->coll_param.comm;
                coll_sched.reset(new ccl_extra_sched(coll_param, sched->sched_id));

                auto result = ccl_coll_build_bcast(coll_sched.get(),
                                                   recv_buf,
                                                   coll_sched->coll_param.count,
                                                   coll_sched->coll_param.dtype,
                                                   coll_sched->coll_param.root);

                CCL_ASSERT(result == ccl_status_success, "bad result ", result);

                break;
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
                coll_sched.reset(new ccl_extra_sched(coll_param, sched->sched_id));
                coll_sched->coll_attr.reduction_fn = sched->coll_attr.reduction_fn;

                auto result = ccl_coll_build_allreduce(coll_sched.get(),
                                                       send_buf,
                                                       recv_buf,
                                                       coll_sched->coll_param.count,
                                                       coll_sched->coll_param.dtype,
                                                       coll_sched->coll_param.reduction);

                CCL_ASSERT(result == ccl_status_success, "bad result ", result);

                break;
            }
            case ccl_coll_allgatherv:
                break;
            default:
                CCL_FATAL("not supported type ", ctype);
                break;
        }

        if (coll_sched)
        {
            LOG_DEBUG("starting COLL entry");
            auto req = sched->start_subsched(coll_sched.get());
            LOG_DEBUG("COLL entry: sched ", coll_sched.get(), ", req ", req);
            // TODO: insert into per-worker sched cache
        }
        else
        {
            CCL_ASSERT(0);
        }
    }

    ccl_coll_type ctype;
    ccl_buffer send_buf;
    ccl_buffer recv_buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    ccl_reduction_t op;
    std::unique_ptr<ccl_extra_sched> coll_sched;
    //for bcast
    size_t root;
};
