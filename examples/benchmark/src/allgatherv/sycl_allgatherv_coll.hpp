#ifndef SYCL_ALLGATHERV_COLL_HPP
#define SYCL_ALLGATHERV_COLL_HPP

#include "allgatherv_strategy.hpp"

#ifdef CCL_ENABLE_SYCL
#include "sycl_coll.hpp"

template<class Dtype>
class allgatherv_buf_check {};

template<class Dtype>
class allatherv_buf_fill {};

template<class Dtype>
struct sycl_allgatherv_coll : sycl_base_coll<Dtype, allgatherv_strategy_impl>
{
    using coll_base = sycl_base_coll<Dtype, allgatherv_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;
    using coll_base::check_values;
    using coll_base::comm;

    sycl_allgatherv_coll() : coll_base(1, base_coll::comm->size(), base_coll::comm->size()) {}

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        size_t local_rank = comm->rank();
        size_t local_size = comm->size();
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            sycl_queue.submit([&](handler& cgh)
            {
                auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx]));
                auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx]));
                auto send_buf_acc = send_buf->template get_access<mode::write>(cgh);
                auto recv_buf_acc = recv_buf->template get_access<mode::write>(cgh);
                cgh.parallel_for<class allatherv_buf_fill<Dtype>>(range<1>{elem_count}, [=](item<1> e_idx)
                {
                    send_buf_acc[e_idx] = local_rank;
                    for (size_t idx = 0; idx < local_size; idx++)
                    {
                        recv_buf_acc[idx * elem_count + e_idx.get_id(0)] = 0;
                    }
                });
            });
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        bool unexpected_device_value = false;
        size_t local_size = comm->size();
        Dtype sbuf_expected = comm->rank();

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            sycl_queue.submit([&](handler& cgh)
            {
                auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx]));
                auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx]));
                auto send_buf_acc = send_buf->template get_access<mode::write>(cgh);
                auto recv_buf_acc = recv_buf->template get_access<mode::write>(cgh);
                cgh.parallel_for<class allgatherv_buf_check<Dtype>>(range<1>{elem_count}, [=](item<1> e_idx) mutable
                {
                    Dtype value = send_buf_acc[e_idx];
                    if (value != sbuf_expected)
                        unexpected_device_value = true;

                    for (size_t idx = 0; idx < local_size; idx++)
                    {
                        Dtype rbuf_expected = idx;
                        value = recv_buf_acc[idx * elem_count + e_idx.get_id(0)];
                        if (value != rbuf_expected)
                            unexpected_device_value = true;
                    }
                });
            });
        }

        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx]));
            auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx]));
            auto send_buf_acc = send_buf->template get_access<mode::write>();
            auto recv_buf_acc = recv_buf->template get_access<mode::write>();

            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = send_buf_acc[e_idx];
                if (value != sbuf_expected)
                {
                    std::cout << this->name() << " send_bufs: buf_idx "
                              << b_idx << ", elem_idx " << e_idx << ", expected "
                              << sbuf_expected << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }

            for (size_t idx = 0; idx < comm->size(); idx++)
            {
                Dtype rbuf_expected = idx;
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    value = recv_buf_acc[idx * elem_count + e_idx];
                    if (value != rbuf_expected)
                    {
                        std::cout << this->name() << " recv_bufs: buf_idx "
                                  << b_idx << ", elem_idx " << e_idx << ", expected "
                                  << rbuf_expected << ", got " << value << std::endl;
                        ASSERT(0, "unexpected value");
                    }
                }
            }
        }

        if (unexpected_device_value)
            ASSERT(0, "unexpected value on device");
    }
};

#endif /* CCL_ENABLE_SYCL */

#endif /* SYCL_ALLGATHERV_COLL_HPP */
