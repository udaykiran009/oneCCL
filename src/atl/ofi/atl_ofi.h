#include "atl.h"
#include <iostream>
#include <memory>

class atl_ofi final : public iatl {
public:
    atl_ofi() = default;

    ~atl_ofi() override;

    static atl_status_t atl_set_env(atl_attr_t* attr);

    atl_status_t atl_init(int* argc,
                          char*** argv,
                          atl_attr_t* attr,
                          const char* main_addr,
                          std::unique_ptr<ipmi>& pmi) override;

    atl_status_t atl_update(std::unique_ptr<ipmi>& pmi) override;

    atl_ep_t** atl_get_eps() override;

    atl_proc_coord_t* atl_get_proc_coord() override;

    int atl_is_resize_enabled() override;

    atl_status_t atl_mr_reg(const void* buf, size_t len, atl_mr_t** mr) override;

    atl_status_t atl_mr_dereg(atl_mr_t* mr) override;

    atl_status_t atl_ep_send(atl_ep_t* ep,
                             const void* buf,
                             size_t len,
                             size_t dst_proc_idx,
                             uint64_t tag,
                             atl_req_t* req) override;

    atl_status_t atl_ep_recv(atl_ep_t* ep,
                             void* buf,
                             size_t len,
                             size_t src_proc_idx,
                             uint64_t tag,
                             atl_req_t* req) override;

    atl_status_t atl_ep_probe(atl_ep_t* ep,
                              size_t src_proc_idx,
                              uint64_t tag,
                              int* found,
                              size_t* recv_len) override;

    atl_status_t atl_ep_allgatherv(atl_ep_t* ep,
                                   const void* send_buf,
                                   size_t send_len,
                                   void* recv_buf,
                                   const int* recv_lens,
                                   const int* offsets,
                                   atl_req_t* req) override;

    atl_status_t atl_ep_allreduce(atl_ep_t* ep,
                                  const void* send_buf,
                                  void* recv_buf,
                                  size_t len,
                                  atl_datatype_t dtype,
                                  atl_reduction_t op,
                                  atl_req_t* req) override;

    atl_status_t atl_ep_alltoall(atl_ep_t* ep,
                                 const void* send_buf,
                                 void* recv_buf,
                                 int len,
                                 atl_req_t* req) override;

    atl_status_t atl_ep_alltoallv(atl_ep_t* ep,
                                  const void* send_buf,
                                  const int* send_lens,
                                  const int* send_offsets,
                                  void* recv_buf,
                                  const int* recv_lens,
                                  const int* recv_offsets,
                                  atl_req_t* req) override;

    atl_status_t atl_ep_barrier(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t atl_ep_bcast(atl_ep_t* ep,
                              void* buf,
                              size_t len,
                              size_t root,
                              atl_req_t* req) override;

    atl_status_t atl_ep_reduce(atl_ep_t* ep,
                               const void* send_buf,
                               void* recv_buf,
                               size_t len,
                               size_t root,
                               atl_datatype_t dtype,
                               atl_reduction_t op,
                               atl_req_t* req) override;

    atl_status_t atl_ep_reduce_scatter(atl_ep_t* ep,
                                       const void* send_buf,
                                       void* recv_buf,
                                       size_t recv_len,
                                       atl_datatype_t dtype,
                                       atl_reduction_t op,
                                       atl_req_t* req) override;

    atl_status_t atl_ep_read(atl_ep_t* ep,
                             void* buf,
                             size_t len,
                             atl_mr_t* mr,
                             uint64_t addr,
                             uintptr_t remote_key,
                             size_t dst_proc_idx,
                             atl_req_t* req) override;

    atl_status_t atl_ep_write(atl_ep_t* ep,
                              const void* buf,
                              size_t len,
                              atl_mr_t* mr,
                              uint64_t addr,
                              uintptr_t remote_key,
                              size_t dst_proc_idx,
                              atl_req_t* req) override;

    atl_status_t atl_ep_wait(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t atl_ep_wait_all(atl_ep_t* ep, atl_req_t* req, size_t count) override;

    atl_status_t atl_ep_cancel(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t atl_ep_poll(atl_ep_t* ep) override;

    atl_status_t atl_ep_check(atl_ep_t* ep, int* is_completed, atl_req_t* req) override;

    atl_status_t atl_finalize() override;

private:
    atl_ctx_t* ctx = nullptr;
    bool is_finalized{ false };
};
