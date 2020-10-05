#pragma once

#include "sched/entry/entry.hpp"
#include "sched/entry/sycl_entry_helper.hpp"

#include <CL/sycl.hpp>

class sycl_copy_host_to_device_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "SYCL_COPY_H2D";
    }

    sycl_copy_host_to_device_entry() = delete;
    sycl_copy_host_to_device_entry(ccl_sched* sched,
                                   ccl_buffer in_buf,
                                   ccl_buffer out_buf,
                                   size_t cnt,
                                   const ccl_datatype& dtype,
                                   const ccl_stream* stream)
            : sched_entry(sched),
              in_buf(in_buf),
              out_buf(out_buf),
              cnt(cnt),
              dtype(dtype),
              stream(stream) {}

    void start() override {

        LOG_DEBUG(class_name(), "in_buf ", in_buf, ", out_buf ", out_buf, ", cnt ", cnt);

        //fill visitor with actual ccl_buffer data


        auto visitor = make_writer_visitor<cl::sycl::access::mode::write>(
            dtype, cnt, 0, in_buf, stream->get_native_stream(), std::ref(out_buf), [this](void* sycl_pointer, size_t bytes) {
                (void)this;
                (void)sycl_pointer;
                (void)bytes;
                // TODO remove fucntion callback
            });
        ccl_tuple_for_each_indexed<ccl_sycle_buffer_one_dim_types>(visitor);


        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "  dtype ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", cnt ",
                           cnt,
                           ", in_buf ",
                           in_buf,
                           ", out_buf ",
                           out_buf,
                           ", native_stream ",
                           stream->to_string(),
                           "\n");
    }

private:
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t cnt;
    ccl_datatype dtype;
    const ccl_stream* stream;
};
