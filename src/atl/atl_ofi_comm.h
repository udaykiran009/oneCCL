#pragma once

#include "atl/atl_wrapper.h"
#include "atl/ofi/atl_ofi.hpp"

class atl_ofi_comm : public iatl_comm {
public:
    ~atl_ofi_comm() override;
    atl_ofi_comm();
    atl_ofi_comm(std::shared_ptr<ikvs_wrapper> k);
    atl_ofi_comm(int total_rank_count,
                 const std::vector<int>& ranks,
                 std::shared_ptr<ikvs_wrapper> k);

    atl_status_t atl_main_addr_reserve(char* main_addr) override {
        return pmi->pmrt_main_addr_reserve(main_addr);
    }

    atl_status_t atl_finalize() override {
        pmi->pmrt_finalize();

        return transport->atl_finalize();
    }

    atl_status_t atl_update() override {
        return transport->atl_update(pmi);
    }

    atl_status_t atl_wait_notification() override {
        return pmi->pmrt_wait_notification();
    }

    atl_status_t atl_set_resize_function(atl_resize_fn_t fn) override {
        return pmi->pmrt_set_resize_function(fn);
    }

    atl_proc_coord_t* atl_get_proc_coord() override {
        return transport->atl_get_proc_coord();
    }

    atl_status_t atl_mr_reg(const void* buf, size_t len, atl_mr_t** mr) override {
        return transport->atl_mr_reg(buf, len, mr);
    }

    atl_status_t atl_mr_dereg(atl_mr_t* mr) override {
        return transport->atl_mr_dereg(mr);
    }

    atl_status_t atl_ep_send(size_t ep_idx,
                             const void* buf,
                             size_t len,
                             int dst_proc_idx,
                             uint64_t tag,
                             atl_req_t* req) override {
        return transport->atl_ep_send(eps[ep_idx], buf, len, dst_proc_idx, tag, req);
    }

    atl_status_t atl_ep_recv(size_t ep_idx,
                             void* buf,
                             size_t len,
                             int src_proc_idx,
                             uint64_t tag,
                             atl_req_t* req) override {
        return transport->atl_ep_recv(eps[ep_idx], buf, len, src_proc_idx, tag, req);
    }

    atl_status_t atl_ep_probe(size_t ep_idx,
                              int src_proc_idx,
                              uint64_t tag,
                              int* found,
                              size_t* recv_len) override {
        return transport->atl_ep_probe(eps[ep_idx], src_proc_idx, tag, found, recv_len);
    }

    atl_status_t atl_ep_allgatherv(size_t ep_idx,
                                   const void* send_buf,
                                   size_t send_len,
                                   void* recv_buf,
                                   const int* recv_lens,
                                   const int* offsets,
                                   atl_req_t* req) override {
        return transport->atl_ep_allgatherv(
            eps[ep_idx], send_buf, send_len, recv_buf, recv_lens, offsets, req);
    }

    atl_status_t atl_ep_allreduce(size_t ep_idx,
                                  const void* send_buf,
                                  void* recv_buf,
                                  size_t len,
                                  atl_datatype_t dtype,
                                  atl_reduction_t op,
                                  atl_req_t* req) override {
        return transport->atl_ep_allreduce(eps[ep_idx], send_buf, recv_buf, len, dtype, op, req);
    }

    atl_status_t atl_ep_alltoall(size_t ep_idx,
                                 const void* send_buf,
                                 void* recv_buf,
                                 int len,
                                 atl_req_t* req) override {
        return transport->atl_ep_alltoall(eps[ep_idx], send_buf, recv_buf, len, req);
    }

    atl_status_t atl_ep_alltoallv(size_t ep_idx,
                                  const void* send_buf,
                                  const int* send_lens,
                                  const int* send_offsets,
                                  void* recv_buf,
                                  const int* recv_lens,
                                  const int* recv_offsets,
                                  atl_req_t* req) override {
        return transport->atl_ep_alltoallv(
            eps[ep_idx], send_buf, send_lens, send_offsets, recv_buf, recv_lens, recv_offsets, req);
    }

    atl_status_t atl_ep_barrier(size_t ep_idx, atl_req_t* req) override {
        return transport->atl_ep_barrier(eps[ep_idx], req);
    }

    atl_status_t atl_ep_bcast(size_t ep_idx,
                              void* buf,
                              size_t len,
                              int root,
                              atl_req_t* req) override {
        return transport->atl_ep_bcast(eps[ep_idx], buf, len, root, req);
    }

    atl_status_t atl_ep_reduce(size_t ep_idx,
                               const void* send_buf,
                               void* recv_buf,
                               size_t len,
                               int root,
                               atl_datatype_t dtype,
                               atl_reduction_t op,
                               atl_req_t* req) override {
        return transport->atl_ep_reduce(eps[ep_idx], send_buf, recv_buf, len, root, dtype, op, req);
    }

    atl_status_t atl_ep_reduce_scatter(size_t ep_idx,
                                       const void* send_buf,
                                       void* recv_buf,
                                       size_t recv_len,
                                       atl_datatype_t dtype,
                                       atl_reduction_t op,
                                       atl_req_t* req) override {
        return transport->atl_ep_reduce_scatter(
            eps[ep_idx], send_buf, recv_buf, recv_len, dtype, op, req);
    }

    atl_status_t atl_ep_read(size_t ep_idx,
                             void* buf,
                             size_t len,
                             atl_mr_t* mr,
                             uint64_t addr,
                             uintptr_t remote_key,
                             int dst_proc_idx,
                             atl_req_t* req) override {
        return transport->atl_ep_read(
            eps[ep_idx], buf, len, mr, addr, remote_key, dst_proc_idx, req);
    }

    atl_status_t atl_ep_write(size_t ep_idx,
                              const void* buf,
                              size_t len,
                              atl_mr_t* mr,
                              uint64_t addr,
                              uintptr_t remote_key,
                              int dst_proc_idx,
                              atl_req_t* req) override {
        return transport->atl_ep_write(
            eps[ep_idx], buf, len, mr, addr, remote_key, dst_proc_idx, req);
    }

    atl_status_t atl_ep_wait(size_t ep_idx, atl_req_t* req) override {
        return transport->atl_ep_wait(eps[ep_idx], req);
    }

    atl_status_t atl_ep_wait_all(size_t ep_idx, atl_req_t* req, size_t count) override {
        return transport->atl_ep_wait_all(eps[ep_idx], req, count);
    }

    atl_status_t atl_ep_cancel(size_t ep_idx, atl_req_t* req) override {
        return transport->atl_ep_cancel(eps[ep_idx], req);
    }

    atl_status_t atl_ep_poll(size_t ep_idx) override {
        return transport->atl_ep_poll(eps[ep_idx]);
    }

    atl_status_t atl_ep_check(size_t ep_idx, atl_req_t* req) override {
        return transport->atl_ep_check(eps[ep_idx], req);
    }

    size_t get_threads_per_process() override {
        return threads_per_process;
    }

    size_t get_ranks_per_process() override {
        return ranks_per_process;
    }

    int get_rank() override {
        return rank;
    }

    int get_size() override {
        return size;
    }

    int get_r2r_color() override {
        return transport->atl_get_proc_coord()->local_idx;
    }

    int get_host_color() override {
        return transport->atl_get_proc_coord()->hostname_hash;
    }

    /*
     * TODO: Temporary change.
     * Need to define correct to unique id
     */
    size_t get_id() override {
        return 0;
    }

private:
    std::shared_ptr<atl_ofi> transport;

    void init_transport();
};
