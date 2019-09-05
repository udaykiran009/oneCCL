#pragma once

#include "sched/entry/entry.hpp"
#include "sched/queue/queue.hpp"

class write_entry : public sched_entry,
                    public postponed_fields<write_entry,
                                            ccl_sched_entry_field_src_mr,
                                            ccl_sched_entry_field_dst_mr>
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "WRITE";
    }

    write_entry() = delete;
    write_entry(ccl_sched* sched,
                ccl_buffer src_buf,
                atl_mr_t* src_mr,
                size_t cnt,
                ccl_datatype_internal_t dtype,
                size_t dst,
                atl_mr_t* dst_mr,
                size_t dst_buf_off) :
        sched_entry(sched), src_buf(src_buf), src_mr(src_mr),
        cnt(cnt), dtype(dtype), dst(dst), dst_mr(dst_mr),
        dst_buf_off(dst_buf_off)
    {
    }

    ~write_entry()
    {
        if (status == ccl_sched_entry_status_started)
        {
            LOG_DEBUG("cancel WRITE entry dst ", dst, ", req ", &req);
            atl_comm_cancel(sched->bin->get_comm_ctx(), &req);
        }
    }

    void start_derived() override
    {
        update_fields();

        LOG_DEBUG("WRITE entry dst ", dst, ", req ", &req);

        CCL_THROW_IF_NOT(src_buf && src_mr && dst_mr, "incorrect values");

        if (!cnt)
        {
            status = ccl_sched_entry_status_complete;
            return;
        }

        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        atl_status_t atl_status = atl_comm_write(sched->bin->get_comm_ctx(), src_buf.get_ptr(bytes),
                                                 bytes, src_mr,
                                                 (uint64_t)dst_mr->buf + dst_buf_off,
                                                 dst_mr->r_key, dst, &req);
        update_status(atl_status);
    }

    void update_derived() override
    {
        int req_status;
        atl_status_t atl_status = atl_comm_check(sched->bin->get_comm_ctx(), &req_status, &req);

        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("WRITE entry failed. atl_status: ", atl_status_to_str(atl_status));
        }

        if (req_status)
            status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

    atl_mr_t*& get_field_ref(field_id_t<ccl_sched_entry_field_src_mr> id)
    {
        return src_mr;
    }

    atl_mr_t*& get_field_ref(field_id_t<ccl_sched_entry_field_dst_mr> id)
    {
        return dst_mr;
    }
protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "dt ", ccl_datatype_get_name(dtype),
                           ", cnt ", cnt,
                           ", src_buf ", src_buf,
                           ", src_mr ", src_mr,
                           ", dst ", dst,
                           ", dst_mr ", dst_mr,
                           ", dst_off ", dst_buf_off,
                           ", comm_id ", sched->coll_param.comm->id(),
                           ", req %p", &req,
                           "\n");
    }

private:
    ccl_buffer src_buf;
    atl_mr_t* src_mr;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    size_t dst;
    atl_mr_t* dst_mr;
    size_t dst_buf_off;
    atl_req_t req{};
};
