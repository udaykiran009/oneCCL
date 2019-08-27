#pragma once

#include "sched/entry/entry.hpp"

#include <CL/sycl.hpp>

class sycl_copy_device_to_host_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "SYCL_COPY_D2H";
    }

    sycl_copy_device_to_host_entry() = delete;
    sycl_copy_device_to_host_entry(ccl_sched* sched,
                                   ccl_buffer in_buf,
                                   ccl_buffer out_buf,
                                   size_t cnt,
                                   ccl_datatype_internal_t dtype,
                                   const ccl_stream* stream) :
                                   sched_entry(sched), in_buf(in_buf), out_buf(out_buf),
                                   cnt(cnt), dtype(dtype), stream(stream)
    {
    }

    void start_derived() override
    {
        size_t bytes = cnt * ccl_datatype_get_size(dtype);
        auto in_buf_acc = static_cast<ccl_sycl_buffer_t*>(in_buf.get_ptr(bytes))->get_access<cl::sycl::access::mode::read>();
        CCL_ASSERT(cnt <= in_buf_acc.get_count());
        auto comp_status = ccl_comp_copy(in_buf_acc.get_pointer(), out_buf.get_ptr(bytes), cnt, dtype);
        CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "  dtype ", dtype,
                           ", cnt ", cnt,
                           ", in_buf ", in_buf,
                           ", out_buf ", out_buf,
                           ", native_stream ", stream->get_native_stream(),
                           "\n");
    }

private:
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t cnt;
    ccl_datatype_internal_t dtype;
    const ccl_stream* stream;
};
