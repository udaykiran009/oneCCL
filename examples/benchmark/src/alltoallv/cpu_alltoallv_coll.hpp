#ifndef CPU_ALLTOALLV_COLL
#define CPU_ALLTOALLV_COLL

#include "cpu_coll.hpp"
#include "alltoallv_strategy.hpp"

template<class Dtype>
struct cpu_alltoallv_coll : cpu_base_coll<Dtype, alltoallv_strategy_impl>
{
    using coll_base = cpu_base_coll<Dtype, alltoallv_strategy_impl>;
    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::single_send_buf;
    using coll_base::single_recv_buf;
    using coll_base::comm;

    cpu_alltoallv_coll(bench_coll_init_attr init_attr) : coll_base(init_attr,
                                                                   coll_base::comm().size(),
                                                                   coll_base::comm().size(),
                                                                   coll_base::comm().size()) {}

    virtual void prepare(size_t elem_count) override
    {
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++)
        {
            for (size_t idx = 0; idx < coll_base::comm().size(); idx++)
            {
                for (size_t e_idx = 0; e_idx < elem_count; e_idx++)
                {
                    ((Dtype*)send_bufs[b_idx])[idx * elem_count + e_idx] = coll_base::comm().rank();
                    ((Dtype*)recv_bufs[b_idx])[idx * elem_count + e_idx] = 0;
                }
            }
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        Dtype sbuf_expected = coll_base::comm().rank();
        Dtype rbuf_expected;
        Dtype value;
        size_t comm_size = coll_base::comm().size();
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++)
        {
            for (size_t e_idx = 0; e_idx < elem_count * comm_size; e_idx++)
            {
                value = ((Dtype*)send_bufs[b_idx])[e_idx];
                rbuf_expected = e_idx / elem_count;
                if (value != sbuf_expected)
                {
                    std::cout << this->name() << " send_bufs: buf_idx "
                              << b_idx << ", elem_idx " << e_idx << ", expected "
                              << sbuf_expected << ", got " << value << std::endl;
                    ASSERT(0, "unexpected value");
                }

                value = ((Dtype*)recv_bufs[b_idx])[e_idx];
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
};

#endif /* CPU_ALLTOALLV_COLL */
