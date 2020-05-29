#ifndef SYCL_COLL_HPP
#define SYCL_COLL_HPP

#include "coll.hpp"

#ifdef CCL_ENABLE_SYCL

#include "sycl_base.hpp"

template<typename Dtype>
using sycl_buffer_t = cl::sycl::buffer<Dtype, 1>;
cl::sycl::queue sycl_queue;

/* sycl-specific base implementation */
template<class Dtype, class strategy>
struct sycl_base_coll : virtual base_coll, private strategy
{
    using coll_strategy = strategy;

    template<class ...Args>
    sycl_base_coll(size_t sbuf_multiplier, size_t rbuf_multiplier, Args&& ...args) :
        coll_strategy(std::forward<Args>(args)...)
    {
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            send_bufs[idx] = new cl::sycl::buffer<Dtype, 1>(ELEM_COUNT * sbuf_multiplier);
            recv_bufs[idx] = new cl::sycl::buffer<Dtype, 1>(ELEM_COUNT * rbuf_multiplier);
        }
        single_send_buf = new cl::sycl::buffer<Dtype, 1>(SINGLE_ELEM_COUNT * sbuf_multiplier);
        single_recv_buf = new cl::sycl::buffer<Dtype, 1>(SINGLE_ELEM_COUNT * rbuf_multiplier);
    }

    sycl_base_coll() : sycl_base_coll(1, 1)
    {
    }

    virtual ~sycl_base_coll()
    {
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            delete static_cast<sycl_buffer_t<Dtype>*>(send_bufs[idx]);
            delete static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[idx]);
        }
        delete static_cast<sycl_buffer_t<Dtype>*>(single_send_buf);
        delete static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf);
    }

    const char* name() const noexcept override
    {
        return coll_strategy::class_name();
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl::coll_attr& attr,
                       req_list_t& reqs) override
    {
        sycl_buffer_t<Dtype> &send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(send_bufs[buf_idx]));
        sycl_buffer_t<Dtype> &recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[buf_idx]));
        coll_strategy::template start_internal<sycl_buffer_t<Dtype> &>(*comm, count,
                                                                       send_buf, recv_buf,
                                                                       attr, stream, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl::coll_attr& attr,
                              req_list_t& reqs) override
    {
        sycl_buffer_t<Dtype> &send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_send_buf));
        sycl_buffer_t<Dtype> &recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf));
        coll_strategy::template start_internal<sycl_buffer_t<Dtype> &>(*comm, count,
                                                                       send_buf, recv_buf,
                                                                       attr, stream, reqs);
    }

    ccl::datatype get_dtype() const override final
    {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }
};
#endif /* CCL_ENABLE_SYCL */

#endif /* SYCL_COLL_HPP */
