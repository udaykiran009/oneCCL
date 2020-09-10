#ifndef CPU_COLL_HPP
#define CPU_COLL_HPP

#include "coll.hpp"

/* cpu-specific base implementation */
template<class Dtype, class strategy>
struct cpu_base_coll : base_coll, protected strategy, cpu_specific_data
{
    using coll_strategy = strategy;

    template<class ...Args>
    cpu_base_coll(bench_coll_init_attr init_attr, size_t sbuf_multiplier,
                  size_t rbuf_multiplier, Args&& ...args) :
        base_coll(init_attr), coll_strategy(std::forward<Args>(args)...)
    {
        int result = 0;

        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT,
                                     base_coll::get_max_elem_count() * sizeof(Dtype) * sbuf_multiplier);
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT,
                                     base_coll::get_max_elem_count() * sizeof(Dtype) * rbuf_multiplier);
        }
        result = posix_memalign((void**)&single_send_buf, ALIGNMENT,
                                 base_coll::get_single_buf_max_elem_count() * sizeof(Dtype) * sbuf_multiplier);
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT,
                                base_coll::get_single_buf_max_elem_count() * sizeof(Dtype) * rbuf_multiplier);
        (void)result;
    }

    cpu_base_coll(bench_coll_init_attr init_attr) : cpu_base_coll(init_attr, 1, 1)
    {
    }

    virtual ~cpu_base_coll()
    {
        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            free(send_bufs[idx]);
            free(recv_bufs[idx]);
        }
        free(single_send_buf);
        free(single_recv_buf);
    }

    const char* name() const noexcept override
    {
        return coll_strategy::class_name();
    }

    virtual void start(size_t count, size_t buf_idx,
                       const bench_coll_exec_attr& attr,
                       req_list_t& reqs) override
    {
        coll_strategy::start_internal(comm(), count,
                                      static_cast<Dtype*>(send_bufs[buf_idx]),
                                      static_cast<Dtype*>(recv_bufs[buf_idx]),
                                      attr, reqs, coll_strategy::get_op_attr(attr));
    }

    virtual void start_single(size_t count,
                              const bench_coll_exec_attr& attr,
                              req_list_t& reqs) override
    {
        coll_strategy::start_internal(comm(), count,
                                      static_cast<Dtype*>(single_send_buf),
                                      static_cast<Dtype*>(single_recv_buf),
                                      attr, reqs, coll_strategy::get_op_attr(attr));
    }

    ccl::datatype get_dtype() const override final
    {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }

    /* global communicator for all cpu collectives */
    static ccl::communicator& comm()
    {
        if (!cpu_specific_data::comm_ptr)
        {
        }
        return *cpu_specific_data::comm_ptr;
    }
};
#endif /* CPU_COLL_HPP */
