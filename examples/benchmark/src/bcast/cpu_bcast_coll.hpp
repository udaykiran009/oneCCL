#ifndef CPU_BCAST_COLL_HPP
#define CPU_BCAST_COLL_HPP

#include "cpu_coll.hpp"
#include "bcast_strategy.hpp"

template<class Dtype>
struct cpu_bcast_coll : cpu_base_coll<Dtype, bcast_strategy_impl>
{
    using coll_base = cpu_base_coll<Dtype, bcast_strategy_impl>;
    using coll_base::recv_bufs;
    using coll_base::single_recv_buf;
    using coll_base::check_values;
    using coll_base::comm;

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                if (comm->rank() == COLL_ROOT)
                    ((Dtype*)recv_bufs[b_idx])[e_idx] = e_idx;
                else
                    ((Dtype*)recv_bufs[b_idx])[e_idx] = 0;
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
                if (value != e_idx)
                {
                    printf("%s: recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           this->name(), b_idx, e_idx,
                           static_cast<float>(e_idx),
                           static_cast<float>(value));
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }
};

#endif /* CPU_BCAST_COLL_HPP */
