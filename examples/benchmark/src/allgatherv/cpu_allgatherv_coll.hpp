#ifndef CPU_ALLGATHERV_COLL_HPP
#define CPU_ALLGATHERV_COLL_HPP

#include "cpu_coll.hpp"
#include "allgatherv_strategy.hpp"

template<class Dtype>
struct cpu_allgatherv_coll : cpu_base_coll<Dtype, allgatherv_strategy_impl>
{
    using coll_base = cpu_base_coll<Dtype, allgatherv_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;
    using coll_base::comm;

    cpu_allgatherv_coll() : coll_base(1, base_coll::comm->size(),  base_coll::comm->size()) {}

    virtual void prepare(size_t elem_count) override
    {
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                ((Dtype*)send_bufs[b_idx])[e_idx] = comm->rank();
            }

            for (size_t idx = 0; idx < comm->size(); idx++)
            {
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx] = 0;
                }
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        Dtype sbuf_expected = comm->rank();
        Dtype value;
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
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
                    value = ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx];
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
    }
};

#endif /* CPU_ALLGATHERV_COLL */
