#pragma once

#include "sched/entry/entry.hpp"
#include "sched/entry/sycl_entry_helper.hpp"

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
                                   const ccl_datatype& dtype,
                                   const ccl_stream* stream) :
                                   sched_entry(sched), in_buf(in_buf), out_buf(out_buf),
                                   cnt(cnt), dtype(dtype), stream(stream)
    {
    }

    void start() override
    {
        //fill visitor with actual ccl_buffer data
        auto visitor = make_visitor<cl::sycl::access::mode::read>(dtype, cnt, in_buf, [this](void* sycl_pointer, size_t bytes)
        {
            auto comp_status = ccl_comp_copy(sycl_pointer, out_buf.get_ptr(bytes), cnt, dtype);
            CCL_ASSERT(comp_status == ccl_status_success, "bad status ", comp_status);

        });

        ccl_tuple_for_each_indexed<ccl_sycle_buffer_one_dim_types>(visitor);
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
                           "  dtype ", global_data.dtypes->name(dtype),
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
    ccl_datatype dtype;
    const ccl_stream* stream;
};
