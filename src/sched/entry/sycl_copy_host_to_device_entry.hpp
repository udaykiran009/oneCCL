#ifdef ENABLE_SYCL
#pragma once

#include "sched/entry/entry.hpp"

#include <CL/sycl.hpp>

class sycl_copy_host_to_device_entry : public sched_entry
{
public:
    static constexpr const char* entry_class_name() noexcept
    {
        return "SYCL_COPY_HOST_TO_DEVICE";
    }

    sycl_copy_host_to_device_entry() = delete;
    sycl_copy_host_to_device_entry(ccl_sched* sched,
                                   void* in_buf,
                                   ccl_sycl_buffer_t* out_buf,
                                   ccl_datatype_internal_t dtype,
                                   const ccl_stream* stream):
                                   sched_entry(sched), dtype(dtype),
                                   in_buf(in_buf), out_buf(out_buf), stream(stream)
    {
    }

    void start_derived() override
    {
        auto out_buf_acc = out_buf->get_access<cl::sycl::access::mode::discard_write>();
        auto comp_status = ccl_comp_copy(in_buf, out_buf_acc.get_pointer(), out_buf_acc.get_count(), dtype);
        CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return entry_class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "  dtype ", dtype,
                           ", in_buf ", in_buf,
                           ", out_buf ", out_buf,
                           ", native_stream ", stream->get_stream(),
                           "\n");
    }

private:
    ccl_datatype_internal_t dtype;
    void* in_buf;
    ccl_sycl_buffer_t* out_buf;
    const ccl_stream* stream;
};
#endif /* ENABLE_SYCL */
