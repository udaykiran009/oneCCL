#ifndef CPU_ALLTOALL_COLL
#define CPU_ALLTOALL_COLL

#include "cpu_coll.hpp"
#include "alltoall_strategy.hpp"

template<class Dtype>
struct cpu_alltoall_coll : cpu_base_coll<Dtype, alltoall_strategy_impl>
{
    using coll_base = cpu_base_coll<Dtype, alltoall_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::stream;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;
    using coll_base::check_values;
    using coll_base::comm;

    cpu_alltoall_coll() : coll_base(base_coll::comm->size(), base_coll::comm->size()) {}

    virtual void prepare(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t idx = 0; idx < comm->size(); idx++)
            {
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    ((Dtype*)send_bufs[b_idx])[idx * elem_count + e_idx] = comm->rank();
                    ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx] = 0;
                }
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        Dtype sbuf_expected = comm->rank();
        Dtype rbuf_expected;
        Dtype value;
        size_t comm_size = comm->size();
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count * comm_size; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
                rbuf_expected = e_idx / elem_count;
                if (value != sbuf_expected)
                {
                    printf("%s: send_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           this->name(), b_idx, e_idx, sbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }

                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
                if (value != rbuf_expected)
                {
                    printf("%s: recv_bufs: buf_idx %zu, elem_idx %zu, expected %f, got %f\n",
                           this->name(), b_idx, e_idx, rbuf_expected, value);
                    ASSERT(0, "unexpected value");
                }
            }
        }
    }
};

#endif /* CPU_ALLTOALL_COLL */
