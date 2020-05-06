#ifndef CPU_SPARSE_ALLREDUCE_COLL_HPP
#define CPU_SPARSE_ALLREDUCE_COLL_HPP

template<class Dtype, class IType,
         template<class> class IndicesDistributorType = sparse_detail::incremental_indices_distributor>
struct cpu_sparse_allreduce_coll : 
        base_sparse_allreduce_coll<Dtype *, IType *, IndicesDistributorType>
{
    using coll_base = base_sparse_allreduce_coll<Dtype *, IType *, IndicesDistributorType>;
    using coll_strategy = typename coll_base::coll_strategy;

    using coll_base::send_bufs;
    using coll_base::recv_bufs;
    using coll_base::stream;
    using coll_base::single_send_buf;
    using coll_base::single_send_ibuf;
    using coll_base::single_recv_buf;
    using coll_base::single_recv_ibuf;
    using coll_base::single_recv_icount;
    using coll_base::single_recv_vcount;
    using coll_base::comm;
    using coll_base::check_values;

    using coll_base::recv_ibufs;
    using coll_base::send_ibufs;
    using coll_base::recv_icount;
    using coll_base::recv_vcount;

    cpu_sparse_allreduce_coll(const std::string& args, 
                              size_t sbuf_size_modifier = 1,
                              size_t rbuf_size_modifier = 1) : coll_base(args)
    {
        int result = 0;
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            result = posix_memalign((void**)&send_bufs[idx], ALIGNMENT,
                                    ELEM_COUNT * sizeof(Dtype) * sbuf_size_modifier);
            result = posix_memalign((void**)&recv_bufs[idx], ALIGNMENT,
                                    ELEM_COUNT * sizeof(Dtype) * rbuf_size_modifier *
                                    base_coll::comm->size());
            if (result != 0)
            {
                std::cerr << __FUNCTION__ << " - posix_memalign(values), error: "
                          << strerror(errno)
                          << ", on buffer idx: " <<  idx << std::endl;
            }

            result = posix_memalign((void**)&recv_ibufs[idx], ALIGNMENT,
                                    ELEM_COUNT * sizeof(IType) * rbuf_size_modifier *
                                    base_coll::comm->size());
            result = posix_memalign((void**)&send_ibufs[idx], ALIGNMENT,
                                    ELEM_COUNT * sizeof(IType) * sbuf_size_modifier);
            if (result != 0)
            {
                std::cerr << __FUNCTION__ << " - posix_memalign(indices), error: "
                          << strerror(errno)
                          << ", on buffer idx: " <<  idx << std::endl;
            }
        }

        result = posix_memalign((void**)&single_send_buf, ALIGNMENT,
                                SINGLE_ELEM_COUNT * sizeof(Dtype) * sbuf_size_modifier);
        result = posix_memalign((void**)&single_recv_buf, ALIGNMENT,
                                SINGLE_ELEM_COUNT * sizeof(Dtype) * rbuf_size_modifier * 
                                base_coll::comm->size());

        result = posix_memalign((void**)&single_send_ibuf, ALIGNMENT,
                                SINGLE_ELEM_COUNT * sizeof(IType) * sbuf_size_modifier);
        result = posix_memalign((void**)&single_recv_ibuf, ALIGNMENT,
                                SINGLE_ELEM_COUNT * sizeof(IType) * rbuf_size_modifier *
                                base_coll::comm->size());

        for( size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            memset(send_bufs[idx], 0, ELEM_COUNT * sizeof(Dtype) * sbuf_size_modifier);
            memset(recv_bufs[idx], 0, ELEM_COUNT * sizeof(Dtype) * rbuf_size_modifier * 
                                      base_coll::comm->size());
            memset(recv_ibufs[idx], 0, ELEM_COUNT * sizeof(IType) * base_coll::comm->size());
            memset(send_ibufs[idx], 0, ELEM_COUNT * sizeof(IType));

        }

        memset(single_send_buf, 0, SINGLE_ELEM_COUNT * sizeof(Dtype) * sbuf_size_modifier);
        memset(single_recv_buf, 0, SINGLE_ELEM_COUNT * sizeof(Dtype) * rbuf_size_modifier * 
                                   base_coll::comm->size());
        memset(single_send_ibuf, 0, SINGLE_ELEM_COUNT * sizeof(IType) * sbuf_size_modifier);
        memset(single_recv_ibuf, 0, SINGLE_ELEM_COUNT * sizeof(IType) * rbuf_size_modifier *
                                    base_coll::comm->size());
    }

    ~cpu_sparse_allreduce_coll()
    {
        for (size_t idx = 0; idx < BUF_COUNT; idx++)
        {
            free(send_bufs[idx]);
            free(recv_bufs[idx]);
            free(recv_ibufs[idx]);
            free(send_ibufs[idx]);
        }

        free(single_send_buf);
        free(single_recv_buf);
        free(single_send_ibuf);
        free(single_recv_ibuf);
    }

    virtual void prepare(size_t elem_count) override
    {
        this->init_distributor({0, elem_count});
        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            sparse_detail::fill_sparse_data(this->get_expected_recv_counts(elem_count),
                                            *this->indices_distributor_impl,
                                            elem_count, send_ibufs[b_idx],
                                            reinterpret_cast<Dtype*>(send_bufs[b_idx]),
                                            reinterpret_cast<Dtype*>(recv_bufs[b_idx]),
                                            recv_icount[b_idx],
                                            recv_vcount[b_idx],
                                            comm->rank());
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        if (!check_values)
            return;

        for (size_t b_idx = 0; b_idx < BUF_COUNT; b_idx++)
        {
            sparse_detail::check_sparse_result(this->get_expected_recv_counts(elem_count),
                                               elem_count,
                                               send_ibufs[b_idx],
                                               static_cast<const Dtype* >(send_bufs[b_idx]),
                                               recv_ibufs[b_idx],
                                               static_cast<const Dtype* >(recv_bufs[b_idx]),
                                               recv_icount[b_idx],
                                               recv_vcount[b_idx],
                                               comm->size(),
                                               comm->rank());

        }
    }

    virtual void start(size_t count, size_t buf_idx,
                       const ccl_coll_attr_t& coll_attr,
                       req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      send_ibufs[buf_idx],
                                      count,
                                      reinterpret_cast<const Dtype *>(send_bufs[buf_idx]),
                                      count,
                                      static_cast<IType**>(&recv_ibufs[buf_idx]),
                                      &recv_icount[buf_idx],
                                      reinterpret_cast<Dtype**>(&recv_bufs[buf_idx]),
                                      &recv_vcount[buf_idx],
                                      coll_attr, stream, reqs);
    }

    virtual void start_single(size_t count,
                              const ccl_coll_attr_t& coll_attr,
                              req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      single_send_ibuf,
                                      count,
                                      reinterpret_cast<const Dtype *>(single_send_buf),
                                      count,
                                      static_cast<IType**>(&single_recv_ibuf),
                                      &single_recv_icount,
                                      reinterpret_cast<Dtype**>(&single_recv_buf),
                                      &single_recv_vcount,
                                      coll_attr, stream, reqs);
    }
};

#endif /* CPU_SPARSE_ALLREDUCE_COLL_HPP */
