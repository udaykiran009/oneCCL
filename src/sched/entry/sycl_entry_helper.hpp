#pragma once

#include "common/utils/tuple.hpp"

template <class Func, cl::sycl::access::mode access_mode>
struct sycl_buffer_reader_visitor {
    sycl_buffer_reader_visitor(const ccl_datatype& dtype,
                        size_t cnt,
                        size_t offset,
                        const ccl_buffer& buf,
                        cl::sycl::queue queue,
                        ccl_buffer& dest_buf,
                        Func f)
            : requested_dtype(dtype),
              requested_cnt(cnt),
              requested_offset(offset),
              requested_buf(buf),
              q(queue),
              to_buf(dest_buf),
              callback(f) {}

    template <size_t index, class specific_sycl_buffer>
    void invoke() {
        if (index == (int)(requested_dtype.idx())) {
            LOG_DEBUG("visitor matched index: ",
                      index,
                      ", ccl: ",
                      ccl::global_data::get().dtypes->name(requested_dtype),
                      ", in: ",
                      __PRETTY_FUNCTION__);

            size_t bytes = requested_cnt * requested_dtype.size();
            /*
            auto out_buf_acc = static_cast<specific_sycl_buffer*>(requested_buf.get_ptr(bytes))
                                   ->template get_access<access_mode>();
            void* out_pointer = out_buf_acc.get_pointer();
            LOG_DEBUG("requested_cnt: ",
                      requested_cnt,
                      ", requested_dtype.size(): ",
                      requested_dtype.size(),
                      ", requested_offset: ",
                      requested_offset,
                      ", bytes: ",
                      bytes,
                      ", out_buf_acc.get_count(): ",
                      out_buf_acc.get_count());
            CCL_ASSERT(requested_cnt <= out_buf_acc.get_count());
            callback((char*)out_pointer + requested_offset, bytes);
            * */
            auto out_buf_acc = static_cast<specific_sycl_buffer*>(requested_buf.get_ptr(bytes));
            auto* dst_ptr = static_cast<typename specific_sycl_buffer::value_type*>(to_buf.get_ptr(bytes));
            {
                specific_sycl_buffer sycl_out(dst_ptr, requested_cnt);
                size_t offset = requested_offset;
                auto e = q.submit([&](cl::sycl::handler &cgh) {
                    auto recv_buf_acc = sycl_out.template get_access<cl::sycl::access::mode::write>(cgh);
                    auto in_buf_acc = out_buf_acc-> template get_access<cl::sycl::access::mode::read>(cgh);
                    cgh.parallel_for<class sycl_copy_device_to_host_entry_kernel>(
                                    cl::sycl::range<1>{ requested_cnt }, [=](cl::sycl::item<1> id) {
                            recv_buf_acc[id] = in_buf_acc[id + offset];
                    });
                    });
                    e.wait();
            }
        }
        else {
            LOG_TRACE("visitor skipped index: ",
                      index,
                      ", ccl: ",
                      ccl::global_data::get().dtypes->name(requested_dtype),
                      ", in: ",
                      __PRETTY_FUNCTION__);
        }
    }
    const ccl_datatype& requested_dtype;
    size_t requested_cnt;
    size_t requested_offset;
    const ccl_buffer& requested_buf;
    cl::sycl::queue q;
    ccl_buffer& to_buf;
    Func callback;
};

template <cl::sycl::access::mode access_mode, class Func>
sycl_buffer_reader_visitor<Func, access_mode> make_reader_visitor(const ccl_datatype& dtype,
                                                    size_t cnt,
                                                    size_t offset,
                                                    const ccl_buffer& buf,
                                                    cl::sycl::queue queue,
                                                    ccl_buffer& dst,
                                                    Func f) {
    return sycl_buffer_reader_visitor<Func, access_mode>(dtype, cnt, offset, buf, queue, dst, f);
}


//
template <class Func, cl::sycl::access::mode access_mode>
struct sycl_buffer_writer_visitor {
    sycl_buffer_writer_visitor(const ccl_datatype& dtype,
                        size_t cnt,
                        size_t offset,
                        const ccl_buffer& in_buf,
                        cl::sycl::queue queue,
                        ccl_buffer& dest_buf,
                        Func f)
            : requested_dtype(dtype),
              requested_cnt(cnt),
              requested_offset(offset),
              requested_buf(in_buf),
              q(queue),
              to_buf(dest_buf),
              callback(f) {}

    template <size_t index, class specific_sycl_buffer>
    void invoke() {
        if (index == (int)(requested_dtype.idx())) {
            LOG_DEBUG("visitor matched index: ",
                      index,
                      ", ccl: ",
                      ccl::global_data::get().dtypes->name(requested_dtype),
                      ", in: ",
                      __PRETTY_FUNCTION__);

            size_t bytes = requested_cnt * requested_dtype.size();
            /*
            auto out_buf_acc = static_cast<specific_sycl_buffer*>(requested_buf.get_ptr(bytes))
                                   ->template get_access<access_mode>();
            void* out_pointer = out_buf_acc.get_pointer();
            LOG_DEBUG("requested_cnt: ",
                      requested_cnt,
                      ", requested_dtype.size(): ",
                      requested_dtype.size(),
                      ", requested_offset: ",
                      requested_offset,
                      ", bytes: ",
                      bytes,
                      ", out_buf_acc.get_count(): ",
                      out_buf_acc.get_count());
            CCL_ASSERT(requested_cnt <= out_buf_acc.get_count());
            callback((char*)out_pointer + requested_offset, bytes);
            * */
            auto in_buf_host = static_cast<typename specific_sycl_buffer::value_type*>(requested_buf.get_ptr(bytes));
            auto* dst_ptr = static_cast<specific_sycl_buffer*>(to_buf.get_ptr(bytes));
            //size_t total_bytes = requested_cnt * requested_dtype.size();
            {
                specific_sycl_buffer sycl_in(in_buf_host, requested_cnt);
                size_t offset = requested_offset;
                auto e = q.submit([&](cl::sycl::handler &cgh) {
                    auto send_buf_acc = sycl_in.template get_access<cl::sycl::access::mode::read>(cgh);
                    auto out_buf_acc = dst_ptr-> template get_access<cl::sycl::access::mode::write>(cgh);
                    cgh.parallel_for<class sycl_copy_device_to_host_entry_kernel>(
                                    cl::sycl::range<1>{ requested_cnt }, [=](cl::sycl::item<1> id) {
                            out_buf_acc[id] = send_buf_acc[id + offset];
                    });
                    });
                    e.wait();
            }
        }
        else {
            LOG_TRACE("visitor skipped index: ",
                      index,
                      ", ccl: ",
                      ccl::global_data::get().dtypes->name(requested_dtype),
                      ", in: ",
                      __PRETTY_FUNCTION__);
        }
    }
    const ccl_datatype& requested_dtype;
    size_t requested_cnt;
    size_t requested_offset;
    const ccl_buffer& requested_buf;
    cl::sycl::queue q;
    ccl_buffer& to_buf;
    Func callback;
};
template <cl::sycl::access::mode access_mode, class Func>
sycl_buffer_writer_visitor<Func, access_mode> make_writer_visitor(const ccl_datatype& dtype,
                                                    size_t cnt,
                                                    size_t offset,
                                                    const ccl_buffer& in_buf,
                                                    cl::sycl::queue queue,
                                                    ccl_buffer& dst_buf,
                                                    Func f) {
    return sycl_buffer_writer_visitor<Func, access_mode>(dtype, cnt, offset, in_buf, queue, dst_buf, f);
}
