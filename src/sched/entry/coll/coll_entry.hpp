#pragma once

#include "common/global/global.hpp"
#include "comp/comp.hpp"
#include "sched/entry/coll/coll_entry_param.hpp"
#include "sched/entry/entry.hpp"

class coll_entry : public sched_entry,
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
            : sched_entry(sched),
              param(param),
              coll_sched(),
              coll_sched_op_id(op_id) {}

    ~coll_entry() {
        coll_sched.reset();
    }

    void start() override;
    void update() override;

    bool is_strict_order_satisfied() override {
        return (coll_sched) ? coll_sched->is_strict_order_satisfied() : false;
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

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(param.dtype),
                           ", coll_type ",
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
                           ", coll sched ",
                           coll_sched.get(),
                           "\n");
    }

private:
    ccl_coll_entry_param param;
    std::unique_ptr<ccl_extra_sched> coll_sched;
    ccl_op_id_t coll_sched_op_id;
};
