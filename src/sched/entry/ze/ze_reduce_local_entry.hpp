#pragma once

#include "sched/entry/entry.hpp"
#include "sched/entry/ze/ze_base_entry.hpp"

class ze_reduce_local_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_REDUCE_LOCAL";
    }

    const char* name() const override {
        return class_name();
    }

    virtual std::string name_ext() const override {
        std::stringstream out;
        out << name() << " ";
        out << "in size: " << in_cnt;
        return out.str();
    }

    explicit ze_reduce_local_entry(ccl_sched* sched,
                                   const ccl_buffer in_buf,
                                   size_t in_cnt,
                                   ccl_buffer inout_buf,
                                   size_t* out_cnt,
                                   const ccl_datatype& dtype,
                                   ccl::reduction op);

    void init_ze_hook() override;
    void finalize_ze_hook() override;

private:
    const ccl_buffer in_buf;
    const size_t in_cnt;
    const ccl_buffer inout_buf;
    const ccl_datatype dtype;
    const ccl::reduction op;

    ze_module_handle_t module{};
    ze_kernel_handle_t kernel{};
    std::string kernel_name{};
};
