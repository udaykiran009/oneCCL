#pragma once

#include "common/global/global.hpp"
#include "comp/comp.hpp"
#include "sched/entry/coll/coll_entry_param.hpp"
#include "sched/entry/subsched_entry.hpp"

class coll_entry : public subsched_entry,
                   public postponed_fields<coll_entry,
                                           ccl_sched_entry_field_send_buf,
                                           ccl_sched_entry_field_recv_buf,
                                           ccl_sched_entry_field_cnt,
                                           ccl_sched_entry_field_dtype,
                                           ccl_sched_entry_field_send_count> {
public:
    static constexpr const char* class_name() noexcept {
        return "COLL";
    }

    coll_entry() = delete;
    coll_entry(ccl_sched* sched, const ccl_coll_entry_param& param, ccl_op_id_t op_id = 0)
            : subsched_entry(
                  sched,
                  op_id,
                  [this](ccl_sched* s) {
                      coll_entry::build_sched(s, this->param);
                  },
                  "coll_entry"),
              param(param) {}

    void start() override;

    bool is_strict_order_satisfied() override {
#ifdef CCL_ENABLE_SYCL
        /* use more strict condition for SYCL build to handle async execution */
        return (status == ccl_sched_entry_status_complete);
#else // CCL_ENABLE_SYCL
        return (subsched) ? subsched->is_strict_order_satisfied() : false;
#endif // CCL_ENABLE_SYCL
    }

    const char* name() const override {
        return class_name();
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_send_buf> id) {
        return param.send_buf;
    }

    ccl_buffer& get_field_ref(field_id_t<ccl_sched_entry_field_recv_buf> id) {
        return param.recv_buf;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_cnt> id) {
        return param.count;
    }

    ccl_datatype& get_field_ref(field_id_t<ccl_sched_entry_field_dtype> id) {
        return param.dtype;
    }

    size_t& get_field_ref(field_id_t<ccl_sched_entry_field_send_count> id) {
        return param.send_count;
    }

    static ccl::status build_sched(ccl_sched* sched, const ccl_coll_entry_param& param);

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(param.dtype),
                           ", coll ",
                           ccl_coll_type_to_str(param.ctype),
                           ", send_buf ",
                           param.send_buf,
                           ", recv_buf ",
                           param.recv_buf,
                           ", cnt ",
                           param.count,
                           ", op ",
                           ccl_reduction_to_str(param.reduction),
                           ", comm ",
                           param.comm,
                           "\n");
    }

private:
    ccl_coll_entry_param param;
};
