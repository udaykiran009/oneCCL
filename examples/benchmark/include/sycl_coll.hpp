#pragma once

#include <iostream>
#include <map>
#include <set>
#include <string>

#include "coll.hpp"
#include "sycl_base.hpp" /* from examples/include */

#ifdef CCL_ENABLE_SYCL

#include <CL/sycl.hpp>

using namespace sycl;
using namespace sycl::access;

std::vector<cl::sycl::queue> get_sycl_queues(sycl_dev_type_t dev_type,
                                             std::vector<size_t>& ranks) {

    std::vector<cl::sycl::queue> queues;

    for (auto rank : ranks) {
        sycl::queue q;
        std::string dev_type_name = sycl_dev_names[dev_type];
        ASSERT(create_sycl_queue_from_device_type(dev_type_name, rank, q),
            "failed to create SYCL queue for dev_type %s and for rank %zu",
            dev_type_name.c_str(), rank);
        queues.push_back(q);
    }

    return queues;
}

/* sycl-specific base implementation */
template <class Dtype, class strategy>
struct sycl_base_coll : base_coll, private strategy {
    using coll_strategy = strategy;

    template <class... Args>
    sycl_base_coll(bench_init_attr init_attr,
                   size_t sbuf_multiplier,
                   size_t rbuf_multiplier,
                   Args&&... args)
            : base_coll(init_attr),
              coll_strategy(std::forward<Args>(args)...) {

        auto& transport = transport_data::instance();
        auto streams = transport.get_streams();

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {

                allocators.push_back(buf_allocator<Dtype>(streams[rank_idx].get_native()));

                auto& allocator = allocators[rank_idx];

                sycl::usm::alloc usm_alloc_type;
                auto bench_alloc_type = base_coll::get_sycl_usm_type();
                if (bench_alloc_type == SYCL_USM_SHARED)
                    usm_alloc_type = usm::alloc::shared;
                else if (bench_alloc_type == SYCL_USM_DEVICE)
                    usm_alloc_type = usm::alloc::device;
                else
                    ASSERT(0, "unexpected bench_alloc_type %d", bench_alloc_type);

                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    send_bufs[idx][rank_idx] = allocator.allocate(
                        base_coll::get_max_elem_count() * sbuf_multiplier, usm_alloc_type);
                    recv_bufs[idx][rank_idx] = allocator.allocate(
                        base_coll::get_max_elem_count() * rbuf_multiplier, usm_alloc_type);
                }

                single_send_buf[rank_idx] = allocator.allocate(
                    base_coll::get_single_buf_max_elem_count() * sbuf_multiplier, usm_alloc_type);
                single_recv_buf[rank_idx] = allocator.allocate(
                    base_coll::get_single_buf_max_elem_count() * rbuf_multiplier, usm_alloc_type);
            }
            else {
                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    send_bufs[idx][rank_idx] =
                        new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * sbuf_multiplier);
                    recv_bufs[idx][rank_idx] =
                        new cl::sycl::buffer<Dtype, 1>(base_coll::get_max_elem_count() * rbuf_multiplier);
                }

                single_send_buf[rank_idx] = new cl::sycl::buffer<Dtype, 1>(
                    base_coll::get_single_buf_max_elem_count() * sbuf_multiplier);

                single_recv_buf[rank_idx] = new cl::sycl::buffer<Dtype, 1>(
                    base_coll::get_single_buf_max_elem_count() * rbuf_multiplier);
            }
        }
    }

    sycl_base_coll(bench_init_attr init_attr) : sycl_base_coll(init_attr, 1, 1) {}

    virtual ~sycl_base_coll() {

        for (size_t rank_idx = 0; rank_idx < base_coll::get_ranks_per_proc(); rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_BUF) {
                for (size_t idx = 0; idx < base_coll::get_buf_count(); idx++) {
                    delete static_cast<sycl_buffer_t<Dtype>*>(send_bufs[idx][rank_idx]);
                    delete static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[idx][rank_idx]);
                }
                delete static_cast<sycl_buffer_t<Dtype>*>(single_send_buf[rank_idx]);
                delete static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf[rank_idx]);
            }
        }
    }

    const char* name() const noexcept override {
        return coll_strategy::class_name();
    }

    virtual void prepare(size_t elem_count) override {
        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
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
        auto& comms = transport.get_device_comms();
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
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {
                coll_strategy::start_internal(
                    comms[rank_idx],
                    count,
                    static_cast<Dtype*>(send_bufs[buf_idx][rank_idx]),
                    static_cast<Dtype*>(recv_bufs[buf_idx][rank_idx]),
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
            else {
                sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(send_bufs[buf_idx][rank_idx]));
                sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(recv_bufs[buf_idx][rank_idx]));
                coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
                    comms[rank_idx],
                    count,
                    send_buf,
                    recv_buf,
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
        }
    }

    virtual void start_single(size_t count,
                              const bench_exec_attr& attr,
                              req_list_t& reqs) override {

        auto& transport = transport_data::instance();
        auto& comms = transport.get_device_comms();
        auto streams = transport.get_streams();
        size_t ranks_per_proc = base_coll::get_ranks_per_proc();

        for (size_t rank_idx = 0; rank_idx < ranks_per_proc; rank_idx++) {

            if (base_coll::get_sycl_mem_type() == SYCL_MEM_USM) {
                coll_strategy::start_internal(
                    comms[rank_idx],
                    count,
                    static_cast<Dtype*>(single_send_buf[rank_idx]),
                    static_cast<Dtype*>(single_recv_buf[rank_idx]),
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
            else
            {
                sycl_buffer_t<Dtype>& send_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_send_buf[rank_idx]));
                sycl_buffer_t<Dtype>& recv_buf = *(static_cast<sycl_buffer_t<Dtype>*>(single_recv_buf[rank_idx]));
                coll_strategy::template start_internal<sycl_buffer_t<Dtype>&>(
                    comms[rank_idx],
                    count,
                    send_buf,
                    recv_buf,
                    attr,
                    reqs,
                    streams[rank_idx],
                    coll_strategy::get_op_attr(attr));
            }
        }
    }

    ccl::datatype get_dtype() const override final {
        return ccl::native_type_info<typename std::remove_pointer<Dtype>::type>::ccl_datatype_value;
    }

private:
    std::vector<buf_allocator<Dtype>> allocators;
};

#endif /* CCL_ENABLE_SYCL */
