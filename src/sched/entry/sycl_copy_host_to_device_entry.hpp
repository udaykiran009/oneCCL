#ifdef ENABLE_SYCL
#pragma once

#include "sched/entry/entry.hpp"
#include "sched/entry_factory.h"

#include <CL/sycl.hpp>

class sycl_copy_host_to_device_entry : public sched_entry
{
public:
    static constexpr const char *entry_class_name() noexcept
    {
        return "SYCL COPY HOST TO DEVICE";
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
        //sycl_copy_host_to_device(stream->stream(), in_buf, out_buf);

        auto out_buf_acc = out_buf->get_access<cl::sycl::access::mode::discard_write>();

        memcpy(out_buf_acc.get_pointer(), in_buf, out_buf_acc.get_count()*ccl_datatype_get_size(dtype));

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


//Factory method specific implementation
namespace entry_factory
{
    namespace detail
    {
        template <>
        class entry_creator<sycl_copy_host_to_device_entry> //force adding barrier before entry creation
        {
        public:
            static sycl_copy_host_to_device_entry* create(ccl_sched* sched,
                                                          void* in_buf,
                                                          ccl_sycl_buffer_t* out_buf,
                                                          ccl_datatype_internal_t dtype,
                                                          const ccl_stream* stream)
            {
                sched->add_barrier();
                auto &&new_entry = std::unique_ptr<sycl_copy_host_to_device_entry>(
                            new sycl_copy_host_to_device_entry(sched, in_buf, out_buf, dtype, stream));
                return static_cast<sycl_copy_host_to_device_entry*>(sched->add_entry(std::move(new_entry)));
            }
        };
    }
}

#endif /* ENABLE_SYCL */
