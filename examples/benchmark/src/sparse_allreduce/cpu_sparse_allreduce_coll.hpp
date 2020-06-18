#ifndef CPU_SPARSE_ALLREDUCE_COLL_HPP
#define CPU_SPARSE_ALLREDUCE_COLL_HPP

template<class VType, class IType,
         template<class> class IndicesDistributorType = sparse_detail::incremental_indices_distributor>
struct cpu_sparse_allreduce_coll : 
        base_sparse_allreduce_coll<VType*, IType*, IndicesDistributorType>
{
    using coll_base = base_sparse_allreduce_coll<VType*, IType*, IndicesDistributorType>;
    using coll_strategy = typename coll_base::coll_strategy;

    using coll_base::stream;
    using coll_base::comm;

    using coll_base::send_ibufs;
    using coll_base::send_vbufs;
    using coll_base::recv_ibufs;
    using coll_base::recv_vbufs;
    using coll_base::recv_icount;
    using coll_base::recv_vcount;
    using coll_base::fn_ctxs;

    using coll_base::single_send_ibuf;
    using coll_base::single_send_vbuf;
    using coll_base::single_recv_ibuf;
    using coll_base::single_recv_vbuf;
    using coll_base::single_recv_icount;
    using coll_base::single_recv_vcount;
    using coll_base::single_fn_ctx;

    cpu_sparse_allreduce_coll(bench_coll_init_attr init_attr,
                              size_t sbuf_size_modifier = 1,
                              size_t rbuf_size_modifier = 1) : coll_base(init_attr)
    {
        int result = 0;

        size_t max_elem_count = base_coll::get_max_elem_count();
        size_t single_buf_max_elem_count = base_coll::get_single_buf_max_elem_count();

        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            result = posix_memalign((void**)&send_ibufs[idx], ALIGNMENT,
                                     max_elem_count * sizeof(IType) * sbuf_size_modifier);
            result |= posix_memalign((void**)&send_vbufs[idx], ALIGNMENT,
                                     max_elem_count * sizeof(VType) * sbuf_size_modifier);
            result |= posix_memalign((void**)&recv_ibufs[idx], ALIGNMENT,
                                     max_elem_count * sizeof(IType) * rbuf_size_modifier *
                                     base_coll::comm->size());
            result |= posix_memalign((void**)&recv_vbufs[idx], ALIGNMENT,
                                     max_elem_count * sizeof(VType) * rbuf_size_modifier *
                                     base_coll::comm->size());
            if (result != 0)
            {
                std::cerr << __FUNCTION__ << " - posix_memalign error: "
                          << strerror(errno)
                          << ", on buffer idx: " <<  idx << std::endl;
            }
        }

        result = posix_memalign((void**)&single_send_ibuf, ALIGNMENT,
                                single_buf_max_elem_count * sizeof(IType) * sbuf_size_modifier);
        result |= posix_memalign((void**)&single_send_vbuf, ALIGNMENT,
                                single_buf_max_elem_count * sizeof(VType) * sbuf_size_modifier);

        result |= posix_memalign((void**)&single_recv_ibuf, ALIGNMENT,
                                single_buf_max_elem_count * sizeof(IType) * rbuf_size_modifier *
                                base_coll::comm->size());
        result |= posix_memalign((void**)&single_recv_vbuf, ALIGNMENT,
                                single_buf_max_elem_count * sizeof(VType) * rbuf_size_modifier *
                                base_coll::comm->size());

        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            std::memset(send_ibufs[idx], 0, max_elem_count * sizeof(IType));
            std::memset(send_vbufs[idx], 0, max_elem_count * sizeof(VType) * sbuf_size_modifier);

            std::memset(recv_ibufs[idx], 0, max_elem_count * sizeof(IType) * rbuf_size_modifier *
                                            base_coll::comm->size());
            std::memset(recv_vbufs[idx], 0, max_elem_count * sizeof(VType) * rbuf_size_modifier *
                                            base_coll::comm->size());
        }

        std::memset(single_send_ibuf, 0, single_buf_max_elem_count * sizeof(IType) * sbuf_size_modifier);
        std::memset(single_send_vbuf, 0, single_buf_max_elem_count * sizeof(VType) * sbuf_size_modifier);

        std::memset(single_recv_ibuf, 0, single_buf_max_elem_count * sizeof(IType) * rbuf_size_modifier *
                                         base_coll::comm->size());
        std::memset(single_recv_vbuf, 0, single_buf_max_elem_count * sizeof(VType) * rbuf_size_modifier *
                                         base_coll::comm->size());

        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            fn_ctxs[idx].recv_ibuf = (void**)(&(recv_ibufs[idx]));
            fn_ctxs[idx].recv_vbuf = (void**)(&(recv_vbufs[idx]));
            fn_ctxs[idx].recv_ibuf_count = max_elem_count * rbuf_size_modifier * base_coll::comm->size();
            fn_ctxs[idx].recv_vbuf_count = max_elem_count * rbuf_size_modifier * base_coll::comm->size();
        }
        single_fn_ctx.recv_ibuf = (void**)(&single_recv_ibuf);
        single_fn_ctx.recv_vbuf = (void**)(&single_recv_vbuf);
        single_fn_ctx.recv_ibuf_count = single_buf_max_elem_count * rbuf_size_modifier * base_coll::comm->size();
        single_fn_ctx.recv_vbuf_count = single_buf_max_elem_count * rbuf_size_modifier * base_coll::comm->size();
    }

    ~cpu_sparse_allreduce_coll()
    {
        for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++)
        {
            free(send_ibufs[idx]);
            free(send_vbufs[idx]);
            free(recv_ibufs[idx]);
            free(recv_vbufs[idx]);
        }

        free(single_send_ibuf);
        free(single_send_vbuf);
        free(single_recv_ibuf);
        free(single_recv_vbuf);
    }

    virtual void prepare(size_t elem_count) override
    {
        this->init_distributor({0, elem_count});
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++)
        {
            sparse_detail::fill_sparse_data(this->get_expected_recv_counts(elem_count),
                                            *this->indices_distributor_impl,
                                            elem_count,
                                            send_ibufs[b_idx],
                                            reinterpret_cast<VType*>(send_vbufs[b_idx]),
                                            reinterpret_cast<VType*>(recv_vbufs[b_idx]),
                                            fn_ctxs[b_idx].recv_vbuf_count,
                                            recv_icount[b_idx],
                                            recv_vcount[b_idx],
                                            comm->rank());
        }
    }

    virtual void finalize(size_t elem_count) override
    {
        for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++)
        {
            sparse_detail::check_sparse_result(this->get_expected_recv_counts(elem_count),
                                               elem_count,
                                               send_ibufs[b_idx],
                                               static_cast<const VType*>(send_vbufs[b_idx]),
                                               recv_ibufs[b_idx],
                                               static_cast<const VType*>(recv_vbufs[b_idx]),
                                               recv_icount[b_idx],
                                               recv_vcount[b_idx],
                                               comm->size(),
                                               comm->rank());
        }
    }

    virtual void start(size_t count, size_t buf_idx,
                       const bench_coll_exec_attr& attr,
                       req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      send_ibufs[buf_idx], count,
                                      send_vbufs[buf_idx], count,
                                      recv_ibufs[buf_idx], recv_icount[buf_idx],
                                      recv_vbufs[buf_idx], recv_vcount[buf_idx],
                                      attr, stream, reqs,
                                      fn_ctxs[buf_idx]);
    }

    virtual void start_single(size_t count,
                              const bench_coll_exec_attr& attr,
                              req_list_t& reqs) override
    {
        coll_strategy::start_internal(*comm,
                                      single_send_ibuf, count,
                                      single_send_vbuf, count,
                                      static_cast<IType*>(single_recv_ibuf), single_recv_icount,
                                      reinterpret_cast<VType*>(single_recv_vbuf), single_recv_vcount,
                                      attr, stream, reqs,
                                      single_fn_ctx);
    }
};

#endif /* CPU_SPARSE_ALLREDUCE_COLL_HPP */
