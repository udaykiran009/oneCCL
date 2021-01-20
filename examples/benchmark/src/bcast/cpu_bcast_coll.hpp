#pragma once

#include "cpu_coll.hpp"
#include "bcast_strategy.hpp"

template <class Dtype>
struct cpu_bcast_coll : cpu_base_coll<Dtype, bcast_strategy_impl> {
    using coll_base = cpu_base_coll<Dtype, bcast_strategy_impl>;
    using coll_base::recv_bufs;

    cpu_bcast_coll(bench_init_attr init_attr) : coll_base(init_attr) {}

    virtual void prepare_internal(size_t elem_count,
                                  ccl::communicator& comm,
                                  ccl::stream& stream,
                                  size_t rank_idx) override {
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++) {
                if (comm.rank() == COLL_ROOT)
                    ((Dtype*)recv_bufs[b_idx][rank_idx])[e_idx] = b_idx;
                else
                    ((Dtype*)recv_bufs[b_idx][rank_idx])[e_idx] = 0;
            }
        }
    }

    virtual void finalize_internal(size_t elem_count,
                                   ccl::communicator& comm,
                                   ccl::stream& stream,
                                   size_t rank_idx) override {
        Dtype value;
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++) {
                value = ((Dtype*)recv_bufs[b_idx][rank_idx])[e_idx];
                if (cast_to_size_t(value) != b_idx) {
                    std::cout << this->name() << " recv_bufs: buf_idx " << b_idx << ", rank_idx "
                              << rank_idx << ", elem_idx " << e_idx << ", expected " << b_idx
                              << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }
};
