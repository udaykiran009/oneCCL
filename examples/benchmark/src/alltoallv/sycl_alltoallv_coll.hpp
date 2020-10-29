#pragma once

#include "alltoallv_strategy.hpp"

#ifdef CCL_ENABLE_SYCL
#include "sycl_coll.hpp"

template <class Dtype>
struct sycl_alltoallv_coll : sycl_base_coll<Dtype, alltoallv_strategy_impl> {
    using coll_base = sycl_base_coll<Dtype, alltoallv_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;

    sycl_alltoallv_coll(bench_init_attr init_attr)
            : coll_base(init_attr,
                        transport_data::get_comm_size(),
                        transport_data::get_comm_size(),
                        transport_data::get_comm_size()) {}

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
                h.parallel_for(range<1>{elem_count * comm.size()}, [=](item<1> e_idx)
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

        sycl::buffer<int> unexpected_device_value(1);
        {
            host_accessor unexpected_val_acc(unexpected_device_value, write_only);
            unexpected_val_acc[0] = 0;
        }

        Dtype sbuf_expected = comm.rank();
        size_t comm_size = comm.size();

        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            stream.get_native().submit([&](handler& h) {
                auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx][rank_idx]));
                auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx][rank_idx]));
                auto send_buf_acc = send_buf->template get_access<mode::read>(h);
                auto recv_buf_acc = recv_buf->template get_access<mode::read>(h);
                auto unexpected_val_acc = unexpected_device_value.template get_access<mode::write>(h);
                h.parallel_for(range<1>{elem_count * comm_size}, [=](item<1> e_idx)
                {
                    Dtype value = send_buf_acc[e_idx];
                    Dtype rbuf_expected = static_cast<Dtype>(e_idx.get_id(0) / elem_count);
                    if (value != sbuf_expected)
                        unexpected_val_acc[0] = 1;

                    value = recv_buf_acc[e_idx];
                    if (value != rbuf_expected)
                        unexpected_val_acc[0] = 1;
                });
            }).wait();
        }

        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            auto send_buf = (static_cast<sycl_buffer_t<Dtype>*>(send_bufs[b_idx][rank_idx]));
            auto recv_buf = (static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[b_idx][rank_idx]));
            auto send_buf_acc = send_buf->template get_access<mode::read>();
            auto recv_buf_acc = recv_buf->template get_access<mode::read>();

            for (size_t e_idx = 0; e_idx < elem_count * comm_size; e_idx++) {
                Dtype value = send_buf_acc[e_idx];
                Dtype rbuf_expected = e_idx / elem_count;
                if (value != sbuf_expected) {
                    std::cout << this->name() << " send_bufs: buf_idx " << b_idx
                              << ", rank_idx " << rank_idx
                              << ", elem_idx " << e_idx
                              << ", expected " << sbuf_expected
                              << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }

                value = recv_buf_acc[e_idx];
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

        {
            host_accessor unexpected_val_acc(unexpected_device_value, read_only);
            if (unexpected_val_acc[0])
                ASSERT(0, "unexpected value on device");
        }
    }
};
#endif /* CCL_ENABLE_SYCL */
