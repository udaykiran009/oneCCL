#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include "sched/entry/ze/ze_reduce_local_entry.hpp"
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
class reduce_local_entry : public ze_reduce_local_entry {
#else
class reduce_local_entry : public sched_entry {
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
public:
    static constexpr const char* class_name() noexcept {
        return "REDUCE_LOCAL";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    reduce_local_entry() = delete;
    explicit reduce_local_entry(ccl_sched* sched,
                                const ccl_buffer in_buf,
                                size_t in_cnt,
                                ccl_buffer inout_buf,
                                size_t* out_cnt,
                                const ccl_datatype& dtype,
                                ccl::reduction op);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    void check_use_device();
    void start_on_device();
#else // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    void check_use_device() {}
    void start_on_device() {}
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    void start_on_host();
    void start() override;

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "dt ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", in_buf ",
                           in_buf,
                           ", in_cnt ",
                           in_cnt,
                           ", inout_buf ",
                           inout_buf,
                           ", out_cnt ",
                           out_cnt,
                           ", op ",
                           ccl_reduction_to_str(op),
                           ", red_fn ",
                           fn,
                           "\n");
    }

private:
    const ccl_buffer in_buf;
    const size_t in_cnt;
    const ccl_buffer inout_buf;
    const size_t* out_cnt;
    const ccl_datatype dtype;
    const ccl::reduction op;
    const ccl::reduction_fn fn;

    bool use_device{};
};
