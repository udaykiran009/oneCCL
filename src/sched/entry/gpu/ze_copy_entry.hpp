#pragma once

#include "sched/entry/copy/copy_helper.hpp"
#include "sched/entry/entry.hpp"
#include "sched/sched.hpp"

#include "sched/entry/gpu/ze_base_entry.hpp"

struct copy_attr;

class ze_copy_entry : public ze_base_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_COPY";
    }

    const char* name() const override {
        return class_name();
    }

    bool is_gpu_entry() const noexcept override {
        return true;
    }

    explicit ze_copy_entry(ccl_sched* sched,
                           ccl_buffer in_buf,
                           ccl_buffer out_buf,
                           size_t count,
                           const ccl_datatype& dtype,
                           copy_attr attr = {});
    ~ze_copy_entry();

    void init();
    void start() override;
    void update() override;
    void finalize() override;

private:
    ccl_sched* const sched;
    ccl_buffer in_buf{};
    ccl_buffer out_buf{};
    const ccl_datatype& dtype;
    const copy_attr attr;
    const size_t buf_size_bytes;
};
