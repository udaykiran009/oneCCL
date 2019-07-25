#ifdef ENABLE_SYCL
#pragma once

#include "sched/entry/entry.hpp"

#include <CL/sycl.hpp>

class sycl_copy_host_to_device_entry : public sched_entry
{
public:
    sycl_copy_host_to_device_entry() = delete;
    sycl_copy_host_to_device_entry(ccl_sched* sched,
                                   void* in_buf,
                                   ccl_sycl_buffer_t* out_buf,
                                   ccl_datatype_internal_t dtype,
                                   const ccl_stream* stream):
                                   sched_entry(sched), dtype(dtype),
                                   in_buf(in_buf), out_buf(out_buf), stream(stream)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        //sycl_copy_host_to_device(stream->stream(), in_buf, out_buf);

        auto out_buf_acc = out_buf->get_access<cl::sycl::access::mode::discard_write>();

        memcpy(out_buf_acc.get_pointer(), in_buf, out_buf_acc.get_count()*ccl_datatype_get_size(dtype));

        status = ccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "SYCL COPY HOST TO DEVICE";
    }

protected:
    void dump_detail(std::stringstream& str) const
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
