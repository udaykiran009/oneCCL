#pragma once

#include "reduce_scatter_strategy.hpp"

#ifdef CCL_ENABLE_SYCL
#include "sycl_coll.hpp"

template <class Dtype>
class reduce_scatter_sbuf_check {};

template <class Dtype>
class reduce_scatter_rbuf_check {};

template <class Dtype>
class reduce_scatter_buf_fill {};

template <class Dtype>
struct sycl_reduce_scatter_coll : sycl_base_coll<Dtype, reduce_scatter_strategy_impl> {
    using coll_base = sycl_base_coll<Dtype, reduce_scatter_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;

    sycl_reduce_scatter_coll(bench_init_attr init_attr)
            : coll_base(init_attr) {}

    virtual void prepare_internal(size_t elem_count,
                         ccl::communicator& comm,
                         ccl::stream& stream,
                         size_t rank_idx) override {

        if (base_coll::get_sycl_mem_type() != SYCL_MEM_BUF)
            return;

        size_t local_rank = comm.rank();
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            stream.get_native().submit([&](handler& h) {
                auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx][rank_idx]));
                auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx][rank_idx]));
                auto send_buf_acc = send_buf->template get_access<mode::write>(h);
                auto recv_buf_acc = recv_buf->template get_access<mode::write>(h);
                h.parallel_for<class reduce_scatter_buf_fill<Dtype>>(range<1>{elem_count}, [=](item<1> e_idx)
                {
                    send_buf_acc[e_idx] = local_rank;
                    recv_buf_acc[e_idx] = 0;
                });
            }).wait();
        }
    }

    virtual void finalize_internal(size_t elem_count,
                          ccl::communicator& comm,
                          ccl::stream& stream,
                          size_t rank_idx) override {

        if (base_coll::get_sycl_mem_type() != SYCL_MEM_BUF)
            return;

        bool unexpected_device_value = false;
        Dtype sbuf_expected = comm.rank();
        Dtype rbuf_expected =
            (comm.size() - 1) * ((float)comm.size() / 2);

        size_t recv_elem_count = elem_count / comm.size();

        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            stream.get_native().submit([&](handler& h) {
                auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx][rank_idx]));
                auto send_buf_acc = send_buf->template get_access<mode::read>(h);
                h.parallel_for<class reduce_scatter_sbuf_check<Dtype>>(range<1>{elem_count}, [=](item<1> e_idx) mutable
                {
                    Dtype value = send_buf_acc[e_idx];
                    if (value != sbuf_expected)
                        unexpected_device_value = true;
                });
            }).wait();

            stream.get_native().submit([&](handler& h) {
                auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx][rank_idx]));
                auto recv_buf_acc = recv_buf->template get_access<mode::read>(h);
                h.parallel_for<class reduce_scatter_rbuf_check<Dtype>>(range<1>{recv_elem_count}, [=](item<1> e_idx) mutable
                {
                    Dtype value = recv_buf_acc[e_idx];
                    if (value != rbuf_expected)
                        unexpected_device_value = true;
                });
            }).wait();
        }

        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx][rank_idx]));
            auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx][rank_idx]));
            auto send_buf_acc = send_buf->template get_access<mode::read>();
            auto recv_buf_acc = recv_buf->template get_access<mode::read>();

            for (size_t e_idx = 0; e_idx < elem_count; e_idx++) {
                Dtype value = send_buf_acc[e_idx];
                if (value != sbuf_expected) {
                    std::cout << this->name() << " send_bufs: buf_idx " << b_idx
                              << ", rank_idx " << rank_idx
                              << ", elem_idx " << e_idx
                              << ", expected " << sbuf_expected
                              << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }

            for (size_t e_idx = 0; e_idx < recv_elem_count; e_idx++) {
                Dtype value = recv_buf_acc[e_idx];
                if (value != rbuf_expected) {
                    std::cout << this->name() << " recv_bufs: buf_idx " << b_idx
                              << ", rank_idx " << rank_idx
                              << ", elem_idx " << e_idx
                              << ", expected " << rbuf_expected
                              << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }
        }

        if (unexpected_device_value)
            ASSERT(0, "unexpected value on device");
    }
};
#endif /* CCL_ENABLE_SYCL */
