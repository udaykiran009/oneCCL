#pragma once

#ifdef CCL_ENABLE_SYCL

#include "sched/entry/entry.hpp"
#include "sched/entry/sycl_entry_helper.hpp"

#include <CL/sycl.hpp>

class sycl_copy_d2h_entry : public sched_entry {
public:

    static constexpr const char* class_name() noexcept {
        return "SYCL_COPY_D2H";
    }

    sycl_copy_d2h_entry() = delete;
    sycl_copy_d2h_entry(ccl_sched* sched,
                        ccl_buffer in_buf,
                        ccl_buffer out_buf,
                        size_t count,
                        const ccl_datatype& dtype,
                        const ccl_stream* stream,
                        size_t offset = 0)
            : sched_entry(sched),
              in_buf(in_buf),
              out_buf(out_buf),
              count(count),
              dtype(dtype),
              stream(stream),
              offset(offset) {}

    void start() override {

        LOG_DEBUG(class_name(), ": in_buf ", in_buf, ", out_buf ", out_buf, ", count ", count);

        auto copier = sycl_copier<sycl_copy_direction::d2h>(
          in_buf, out_buf, count, dtype, stream->get_native_stream(), offset);

        ccl_tuple_for_each_indexed<ccl_sycle_buffer_one_dim_types>(copier);
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
                           ", count ",
                           count,
                           ", in_buf ",
                           in_buf,
                           ", out_buf ",
                           out_buf,
                           ", native_stream ",
                           stream->to_string(),
                           ", offset ",
                           offset,
                           "\n");
    }

private:
    ccl_buffer in_buf;
    ccl_buffer out_buf;
    size_t count;
    ccl_datatype dtype;
    const ccl_stream* stream;
    size_t offset;
};

class sycl_copy_h2d_entry : public sched_entry {
public:

    static constexpr const char* class_name() noexcept {
        return "SYCL_COPY_H2D";
    }

    sycl_copy_h2d_entry() = delete;
    sycl_copy_h2d_entry(ccl_sched* sched,
                        ccl_buffer in_buf,
                        ccl_buffer out_buf,
                        size_t count,
                        const ccl_datatype& dtype,
                        const ccl_stream* stream)
            : sched_entry(sched),
              in_buf(in_buf),
              out_buf(out_buf),
              count(count),
              dtype(dtype),
              stream(stream) {}

    void start() override {

        LOG_DEBUG(class_name(), ": in_buf ", in_buf, ", out_buf ", out_buf, ", count ", count);

        auto copier = sycl_copier<sycl_copy_direction::h2d>(
          in_buf, out_buf, count, dtype, stream->get_native_stream(), 0);

        ccl_tuple_for_each_indexed<ccl_sycle_buffer_one_dim_types>(copier);
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
                           ", count ",
                           count,
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
    size_t count;
    ccl_datatype dtype;
    const ccl_stream* stream;
};

#endif /* CCL_ENABLE_SYCL */
