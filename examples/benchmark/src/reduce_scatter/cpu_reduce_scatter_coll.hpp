#pragma once

#include "cpu_coll.hpp"
#include "reduce_scatter_strategy.hpp"

template <class Dtype>
struct cpu_reduce_scatter_coll : cpu_base_coll<Dtype, reduce_scatter_strategy_impl> {
    using coll_base = cpu_base_coll<Dtype, reduce_scatter_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;

    cpu_reduce_scatter_coll(bench_init_attr init_attr) : coll_base(init_attr) {}

    virtual void finalize_internal(size_t elem_count,
                                   ccl::communicator& comm,
                                   ccl::stream& stream,
                                   size_t rank_idx) override {
        Dtype sbuf_expected = get_val<Dtype>(static_cast<float>(comm.rank()));
        Dtype rbuf_expected = get_val<Dtype>((comm.size() - 1) * ((float)comm.size() / 2));
        Dtype value;

        size_t recv_elem_count = elem_count / comm.size();

        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++) {
                value = ((Dtype*)send_bufs[b_idx][rank_idx])[e_idx];
                if (value != sbuf_expected) {
                    std::cout << this->name() << " send_bufs: buf_idx " << b_idx << ", rank_idx "
                              << rank_idx << ", elem_idx " << e_idx << ", expected "
                              << sbuf_expected << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }

            for (size_t e_idx = 0; e_idx < recv_elem_count; e_idx++) {
                value = ((Dtype*)recv_bufs[b_idx][rank_idx])[e_idx];
                if (base_coll::check_error<Dtype>(value, rbuf_expected, comm)) {
                    std::cout << this->name() << " recv_bufs: buf_idx " << b_idx << ", rank_idx "
                              << rank_idx << ", elem_idx " << e_idx << ", expected "
                              << rbuf_expected << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }
};
