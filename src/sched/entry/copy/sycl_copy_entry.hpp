#pragma once

#ifdef CCL_ENABLE_SYCL

#include "sched/entry/copy/copy_helper.hpp"
#include "sched/entry/entry.hpp"

#include <CL/sycl.hpp>

class sycl_copy_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "SYCL_COPY";
    }

    sycl_copy_entry() = delete;
    sycl_copy_entry(ccl_sched* sched,
                    copy_direction direction,
                    ccl_buffer in_buf,
                    ccl_buffer out_buf,
                    size_t count,
                    const ccl_datatype& dtype,
                    const ccl_stream* stream,
                    bool is_sycl_buffer = false,
                    size_t offset = 0)
            : sched_entry(sched),
              direction(direction),
              in_buf(in_buf),
              out_buf(out_buf),
              count(count),
              dtype(dtype),
              stream(stream),
              is_sycl_buffer(is_sycl_buffer),
              offset(offset),
              copier(
                  sycl_copier(direction, in_buf, out_buf, count, dtype, is_sycl_buffer, offset)) {}

    void start() override {
        LOG_DEBUG(class_name(), ": in_buf ", in_buf, ", out_buf ", out_buf, ", count ", count);

        copier.set_queue(((ccl_stream*)stream)->get_native_stream(sched->queue->get_idx()));
        ccl_tuple_for_each_indexed<ccl_sycl_buffer_one_dim_types>(copier);
        status = ccl_sched_entry_status_started;

        // while (!copier.is_completed()) {
        //   ccl_yield(ccl_yield_sleep);
        // }

        // status = ccl_sched_entry_status_complete;
    }

    void update() override {
        if (copier.is_completed()) {
            status = ccl_sched_entry_status_complete;
        }
    }

    const char* name() const override {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "direction ",
                           to_string(direction),
                           ", dtype ",
                           ccl::global_data::get().dtypes->name(dtype),
                           ", count ",
                           count,
                           ", in_buf ",
                           in_buf,
                           ", out_buf ",
                           out_buf,
                           ", native_stream ",
                           stream->to_string(),
                           ", is_sycl_buffer ",
                           is_sycl_buffer,
                           ", offset ",
                           offset,
                           "\n");
    }

private:
    copy_direction direction;
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t count;
    ccl_datatype dtype;
    const ccl_stream* stream;
    bool is_sycl_buffer;
    size_t offset;
    sycl_copier copier;
};

#endif /* CCL_ENABLE_SYCL */
