#pragma once

#include "coll.hpp"

/* cpu-specific base implementation */
template <class Dtype, class strategy>
struct cpu_base_coll : base_coll, protected strategy {
    using coll_strategy = strategy;

    template <class... Args>
    cpu_base_coll(bench_init_attr init_attr,
                  size_t sbuf_multiplier,
                  size_t rbuf_multiplier,
                  Args&&... args)
            : base_coll(init_attr),
              coll_strategy(std::forward<Args>(args)...) {
        int result = 0;

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                result =
                    posix_memalign((void**)&(send_bufs[idx][rank_idx]),
                                   ALIGNMENT,
                                   base_coll::get_max_elem_count() * sizeof(Dtype) * sbuf_multiplier);
                result =
                    posix_memalign((void**)&(recv_bufs[idx][rank_idx]),
                                   ALIGNMENT,
                                   base_coll::get_max_elem_count() * sizeof(Dtype) * rbuf_multiplier);
            }

            result = posix_memalign(
                (void**)&(single_send_buf[rank_idx]),
                ALIGNMENT,
                base_coll::get_single_buf_max_elem_count() * sizeof(Dtype) * sbuf_multiplier);
            result = posix_memalign(
                (void**)&(single_recv_buf[rank_idx]),
                ALIGNMENT,
                base_coll::get_single_buf_max_elem_count() * sizeof(Dtype) * rbuf_multiplier);
            (void)result;
        }
    }

    cpu_base_coll(bench_init_attr init_attr) : cpu_base_coll(init_attr, 1, 1) {}

    virtual ~cpu_base_coll() {

      for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                free(send_bufs[idx][rank_idx]);
                free(recv_bufs[idx][rank_idx]);
            }
            free(single_send_buf[rank_idx]);
            free(single_recv_buf[rank_idx]);
        }
    }

    const char* name() const noexcept override {
        return coll_strategy::class_name();
    }

    virtual void prepare(size_t elem_count) override {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_host_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            prepare_internal(elem_count,
                             comms[rank_idx],
                             streams[rank_idx],
                             rank_idx);
        }
    }

    virtual void finalize(size_t elem_count) override {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_host_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            finalize_internal(elem_count,
                              comms[rank_idx],
                              streams[rank_idx],
                              rank_idx);
        }
    }

    virtual void start(size_t count,
                       size_t buf_idx,
                       const bench_exec_attr& attr,
                       req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_host_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            coll_strategy::start_internal(comms[rank_idx],
                                          count,
                                          static_cast<Dtype*>(send_bufs[buf_idx][rank_idx]),
                                          static_cast<Dtype*>(recv_bufs[buf_idx][rank_idx]),
                                          attr,
                                          reqs,
                                          coll_strategy::get_op_attr(attr));
        }
    }

    virtual void start_single(size_t count,
                              const bench_exec_attr& attr,
                              req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_host_comms();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {
            coll_strategy::start_internal(comms[rank_idx],
                                          count,
                                          static_cast<Dtype*>(single_send_buf[rank_idx]),
                                          static_cast<Dtype*>(single_recv_buf[rank_idx]),
                                          attr,
                                          reqs,
                                          coll_strategy::get_op_attr(attr));
        }
    }

    ccl::datatype get_dtype() const override final {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }
};
