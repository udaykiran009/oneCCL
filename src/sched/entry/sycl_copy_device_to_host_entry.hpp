#ifdef ENABLE_SYCL
#pragma once

#include "sched/entry/entry.hpp"

#include <CL/sycl.hpp>

class sycl_copy_device_to_host_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "SYCL COPY DEVICE TO HOST";
    }

    sycl_copy_device_to_host_entry() = delete;
    sycl_copy_device_to_host_entry(ccl_sched* sched,
                                   ccl_sycl_buffer_t* in_buf,
                                   void* out_buf,
                                   ccl_datatype_internal_t dtype,
                                   const ccl_stream* stream):
                                   sched_entry(sched), dtype(dtype),
                                   in_buf(in_buf), out_buf(out_buf), stream(stream)
    {
    }

    void start_derived() override
    {
        //sycl_copy_device_to_host(stream->stream(), in_buf, *out_buf);

        auto in_buf_acc = in_buf->get_access<cl::sycl::access::mode::read>();

        assert(out_buf);
        memcpy(out_buf, in_buf_acc.get_pointer(), in_buf_acc.get_count()*ccl_datatype_get_size(dtype));

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
    ccl_sycl_buffer_t* in_buf;
    void* out_buf;
    const ccl_stream* stream;
};
#endif /* ENABLE_SYCL */
