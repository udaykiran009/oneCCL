#if 0

#pragma once

template <class VType,
          class IType,
          template <class> class IndicesDistributorType =
              sparse_detail::incremental_indices_distributor>
struct cpu_sparse_allreduce_coll
        : base_sparse_allreduce_coll<VType*, IType*, IndicesDistributorType> {
    using coll_base = base_sparse_allreduce_coll<VType*, IType*, IndicesDistributorType>;
    using coll_strategy = typename coll_base::coll_strategy;

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

    cpu_sparse_allreduce_coll(bench_init_attr init_attr,
                              size_t sbuf_size_modifier = 1,
                              size_t rbuf_size_modifier = 1)
            : coll_base(init_attr, transport_data::get_comm_size()) {
        int result = 0;

        size_t comm_size = transport_data::get_comm_size();

        size_t max_elem_count = base_coll::get_max_elem_count();
        size_t single_buf_max_elem_count = base_coll::get_single_buf_max_elem_count();

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                result = posix_memalign((void**)&(send_ibufs[idx][rank_idx]),
                                        ALIGNMENT,
                                        max_elem_count * sizeof(IType) * sbuf_size_modifier);
                result |= posix_memalign((void**)&(send_vbufs[idx][rank_idx]),
                                         ALIGNMENT,
                                         max_elem_count * sizeof(VType) * sbuf_size_modifier);
                result |=
                    posix_memalign((void**)&(recv_ibufs[idx][rank_idx]),
                                   ALIGNMENT,
                                   max_elem_count * sizeof(IType) * rbuf_size_modifier * comm_size);
                result |=
                    posix_memalign((void**)&(recv_vbufs[idx][rank_idx]),
                                   ALIGNMENT,
                                   max_elem_count * sizeof(VType) * rbuf_size_modifier * comm_size);
                if (result != 0) {
                    std::cerr << __FUNCTION__ << " - posix_memalign error: " << strerror(errno)
                              << ", on buffer idx: " << idx << std::endl;
                }
            }

            result = posix_memalign((void**)&(single_send_ibuf[rank_idx]),
                                    ALIGNMENT,
                                    single_buf_max_elem_count * sizeof(IType) * sbuf_size_modifier);
            result |= posix_memalign((void**)&(single_send_vbuf[rank_idx]),
                                     ALIGNMENT,
                                     single_buf_max_elem_count * sizeof(VType) * sbuf_size_modifier);

            result |= posix_memalign(
                (void**)&(single_recv_ibuf[rank_idx]),
                ALIGNMENT,
                single_buf_max_elem_count * sizeof(IType) * rbuf_size_modifier * comm_size);
            result |= posix_memalign(
                (void**)&(single_recv_vbuf[rank_idx]),
                ALIGNMENT,
                single_buf_max_elem_count * sizeof(VType) * rbuf_size_modifier * comm_size);

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                std::memset(send_ibufs[idx][rank_idx], 0, max_elem_count * sizeof(IType));
                std::memset(send_vbufs[idx][rank_idx], 0, max_elem_count * sizeof(VType) * sbuf_size_modifier);

                std::memset(recv_ibufs[idx][rank_idx],
                            0,
                            max_elem_count * sizeof(IType) * rbuf_size_modifier * comm_size);
                std::memset(recv_vbufs[idx][rank_idx],
                            0,
                            max_elem_count * sizeof(VType) * rbuf_size_modifier * comm_size);
            }

            std::memset(
                single_send_ibuf[rank_idx], 0, single_buf_max_elem_count * sizeof(IType) * sbuf_size_modifier);
            std::memset(
                single_send_vbuf[rank_idx], 0, single_buf_max_elem_count * sizeof(VType) * sbuf_size_modifier);

            std::memset(single_recv_ibuf[rank_idx],
                        0,
                        single_buf_max_elem_count * sizeof(IType) * rbuf_size_modifier * comm_size);
            std::memset(single_recv_vbuf[rank_idx],
                        0,
                        single_buf_max_elem_count * sizeof(VType) * rbuf_size_modifier * comm_size);

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                fn_ctxs[idx][rank_idx].recv_ibuf = (void**)(&(recv_ibufs[idx][rank_idx]));
                fn_ctxs[idx][rank_idx].recv_vbuf = (void**)(&(recv_vbufs[idx][rank_idx]));
                fn_ctxs[idx][rank_idx].recv_ibuf_count = max_elem_count * rbuf_size_modifier * comm_size;
                fn_ctxs[idx][rank_idx].recv_vbuf_count = max_elem_count * rbuf_size_modifier * comm_size;
            }
            single_fn_ctx[rank_idx].recv_ibuf = (void**)(&single_recv_ibuf[rank_idx]);
            single_fn_ctx[rank_idx].recv_vbuf = (void**)(&single_recv_vbuf[rank_idx]);

            single_fn_ctx[rank_idx].recv_ibuf_count =
                single_buf_max_elem_count * rbuf_size_modifier * comm_size;
            single_fn_ctx[rank_idx].recv_vbuf_count =
                single_buf_max_elem_count * rbuf_size_modifier * comm_size;
        }
    }

    ~cpu_sparse_allreduce_coll() {

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                free(send_ibufs[idx][rank_idx]);
                free(send_vbufs[idx][rank_idx]);
                free(recv_ibufs[idx][rank_idx]);
                free(recv_vbufs[idx][rank_idx]);
            }

            free(single_send_ibuf[rank_idx]);
            free(single_send_vbuf[rank_idx]);
            free(single_recv_ibuf[rank_idx]);
            free(single_recv_vbuf[rank_idx]);
        }
    }

    virtual void prepare(size_t elem_count) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            auto& comm = comms[rank_idx];

            this->init_distributor({ 0, elem_count });
            for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
                sparse_detail::fill_sparse_data(this->get_expected_recv_counts(elem_count),
                                                *this->indices_distributor_impl,
                                                elem_count,
                                                send_ibufs[b_idx][rank_idx],
                                                reinterpret_cast<VType*>(send_vbufs[b_idx][rank_idx]),
                                                reinterpret_cast<VType*>(recv_vbufs[b_idx][rank_idx]),
                                                fn_ctxs[b_idx][rank_idx].recv_vbuf_count,
                                                recv_icount[b_idx],
                                                recv_vcount[b_idx],
                                                comm.rank());
            }
        }
    }

    virtual void finalize(size_t elem_count) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            auto& comm = comms[rank_idx];

            for (size_t b_idx = 0; b_idx < base_coll::get_buf_count(); b_idx++) {
                sparse_detail::check_sparse_result(this->get_expected_recv_counts(elem_count),
                                                   elem_count,
                                                   send_ibufs[b_idx][rank_idx],
                                                   static_cast<const VType*>(send_vbufs[b_idx][rank_idx]),
                                                   recv_ibufs[b_idx][rank_idx],
                                                   static_cast<const VType*>(recv_vbufs[b_idx][rank_idx]),
                                                   recv_icount[b_idx],
                                                   recv_vcount[b_idx],
                                                   comm.size(),
                                                   comm.rank());
            }
        }
    }

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_exec_attr& attr,
                       req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            auto& comm = comms[rank_idx];

            coll_strategy::start_internal(comm,
                                          send_ibufs[buf_idx][rank_idx],
                                          count,
                                          send_vbufs[buf_idx][rank_idx],
                                          count,
                                          recv_ibufs[buf_idx][rank_idx],
                                          recv_icount[buf_idx],
                                          recv_vbufs[buf_idx][rank_idx],
                                          recv_vcount[buf_idx],
                                          attr,
                                          reqs,
                                          fn_ctxs[buf_idx][rank_idx],
                                          coll_strategy::get_op_attr(attr));
        }
    }

    virtual void start_single(size_t count,
                              const bench_exec_attr& attr,
                              req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            auto& comm = comms[rank_idx];

            coll_strategy::start_internal(comm,
                                          single_send_ibuf[rank_idx],
                                          count,
                                          single_send_vbuf[rank_idx],
                                          count,
                                          static_cast<IType*>(single_recv_ibuf[rank_idx]),
                                          single_recv_icount,
                                          reinterpret_cast<VType*>(single_recv_vbuf[rank_idx]),
                                          single_recv_vcount,
                                          attr,
                                          reqs,
                                          single_fn_ctx[rank_idx],
                                          coll_strategy::get_op_attr(attr));
        }
    }
};

#endif