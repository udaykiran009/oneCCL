#pragma once
#ifdef CCL_ENABLE_MPI

#include "atl.h"

class atl_mpi final : public iatl {
public:
    atl_mpi() = default;
    ~atl_mpi() override;

    static atl_status_t atl_set_env(const atl_attr_t& attr);

    atl_status_t init(int* argc,
                      char*** argv,
                      atl_attr_t* attr,
                      const char* main_addr,
                      std::unique_ptr<ipmi>& pmi) override;

    atl_status_t update(std::unique_ptr<ipmi>& pmi) override;

    atl_ep_t** get_eps() override;

    atl_proc_coord_t* get_proc_coord() override;

    atl_status_t mr_reg(const void* buf, size_t len, atl_mr_t** mr) override;

    atl_status_t mr_dereg(atl_mr_t* mr) override;

    atl_status_t send(atl_ep_t* ep,
                      const void* buf,
                      size_t len,
                      int dst_proc_idx,
                      uint64_t tag,
                      atl_req_t* req) override;

    atl_status_t recv(atl_ep_t* ep,
                      void* buf,
                      size_t len,
                      int src_proc_idx,
                      uint64_t tag,
                      atl_req_t* req) override;

    atl_status_t probe(atl_ep_t* ep,
                       int src_proc_idx,
                       uint64_t tag,
                       int* found,
                       size_t* recv_len) override;

    atl_status_t allgatherv(atl_ep_t* ep,
                            const void* send_buf,
                            size_t send_len,
                            void* recv_buf,
                            const int* recv_lens,
                            const int* offsets,
                            atl_req_t* req) override;

    atl_status_t allreduce(atl_ep_t* ep,
                           const void* send_buf,
                           void* recv_buf,
                           size_t len,
                           atl_datatype_t dtype,
                           atl_reduction_t op,
                           atl_req_t* req) override;

    atl_status_t alltoall(atl_ep_t* ep,
                          const void* send_buf,
                          void* recv_buf,
                          int len,
                          atl_req_t* req) override;

    atl_status_t alltoallv(atl_ep_t* ep,
                           const void* send_buf,
                           const int* send_lens,
                           const int* send_offsets,
                           void* recv_buf,
                           const int* recv_lens,
                           const int* recv_offsets,
                           atl_req_t* req) override;

    atl_status_t barrier(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t bcast(atl_ep_t* ep, void* buf, size_t len, int root, atl_req_t* req) override;

    atl_status_t reduce(atl_ep_t* ep,
                        const void* send_buf,
                        void* recv_buf,
                        size_t len,
                        int root,
                        atl_datatype_t dtype,
                        atl_reduction_t op,
                        atl_req_t* req) override;

    atl_status_t reduce_scatter(atl_ep_t* ep,
                                const void* send_buf,
                                void* recv_buf,
                                size_t recv_len,
                                atl_datatype_t dtype,
                                atl_reduction_t op,
                                atl_req_t* req) override;

    atl_status_t read(atl_ep_t* ep,
                      void* buf,
                      size_t len,
                      atl_mr_t* mr,
                      uint64_t addr,
                      uintptr_t remote_key,
                      int dst_proc_idx,
                      atl_req_t* req) override;

    atl_status_t write(atl_ep_t* ep,
                       const void* buf,
                       size_t len,
                       atl_mr_t* mr,
                       uint64_t addr,
                       uintptr_t remote_key,
                       int dst_proc_idx,
                       atl_req_t* req) override;

    atl_status_t wait(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t wait_all(atl_ep_t* ep, atl_req_t* req, size_t count) override;

    atl_status_t cancel(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t poll(atl_ep_t* ep) override;

    atl_status_t check(atl_ep_t* ep, atl_req_t* req) override;

    atl_status_t finalize() override;

    int get_rank() {
        return ctx->coord.global_idx;
    }
    int get_size() {
        return ctx->coord.global_count;
    }
    bool is_inited() override {
        return inited;
    }

private:
    atl_ctx_t* ctx = nullptr;
    bool is_finalized{ false };
    bool inited{ false };
};

#endif // CCL_ENABLE_MPI
