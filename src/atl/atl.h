#pragma once

#include <stddef.h>
#include <stdint.h>
#include <memory>

#include "atl_def.h"
#include "common/log/log.hpp"
#include "util/pm/pm_rt.h"

#ifdef __cplusplus
class iatl {
public:
    virtual ~iatl() = default;

    virtual atl_status_t init(int* argc,
                              char*** argv,
                              atl_attr_t* attr,
                              const char* main_addr,
                              std::shared_ptr<ipmi> pmi) = 0;

    virtual atl_status_t finalize() = 0;

    virtual atl_status_t update(std::shared_ptr<ipmi> pmi) = 0;

    virtual atl_ep_t** get_eps() = 0;

    virtual atl_proc_coord_t* get_proc_coord() = 0;

    virtual atl_status_t mr_reg(const void* buf, size_t len, atl_mr_t** mr) = 0;

    virtual atl_status_t mr_dereg(atl_mr_t* mr) = 0;

    virtual atl_status_t send(atl_ep_t* ep,
                              const void* buf,
                              size_t len,
                              int dst_proc_idx,
                              uint64_t tag,
                              atl_req_t* req) = 0;

    virtual atl_status_t recv(atl_ep_t* ep,
                              void* buf,
                              size_t len,
                              int src_proc_idx,
                              uint64_t tag,
                              atl_req_t* req) = 0;

    virtual atl_status_t probe(atl_ep_t* ep,
                               int src_proc_idx,
                               uint64_t tag,
                               int* found,
                               size_t* recv_len) = 0;

    virtual atl_status_t allgatherv(atl_ep_t* ep,
                                    const void* send_buf,
                                    size_t send_len,
                                    void* recv_buf,
                                    const int* recv_lens,
                                    const int* offsets,
                                    atl_req_t* req) = 0;

    virtual atl_status_t allreduce(atl_ep_t* ep,
                                   const void* send_buf,
                                   void* recv_buf,
                                   size_t len,
                                   atl_datatype_t dtype,
                                   atl_reduction_t op,
                                   atl_req_t* req) = 0;

    virtual atl_status_t alltoall(atl_ep_t* ep,
                                  const void* send_buf,
                                  void* recv_buf,
                                  int len,
                                  atl_req_t* req) = 0;

    virtual atl_status_t alltoallv(atl_ep_t* ep,
                                   const void* send_buf,
                                   const int* send_lens,
                                   const int* send_offsets,
                                   void* recv_buf,
                                   const int* recv_lens,
                                   const int* recv_offsets,
                                   atl_req_t* req) = 0;

    virtual atl_status_t barrier(atl_ep_t* ep, atl_req_t* req) = 0;

    virtual atl_status_t bcast(atl_ep_t* ep, void* buf, size_t len, int root, atl_req_t* req) = 0;

    virtual atl_status_t reduce(atl_ep_t* ep,
                                const void* send_buf,
                                void* recv_buf,
                                size_t len,
                                int root,
                                atl_datatype_t dtype,
                                atl_reduction_t op,
                                atl_req_t* req) = 0;

    virtual atl_status_t reduce_scatter(atl_ep_t* ep,
                                        const void* send_buf,
                                        void* recv_buf,
                                        size_t recv_len,
                                        atl_datatype_t dtype,
                                        atl_reduction_t op,
                                        atl_req_t* req) = 0;

    virtual atl_status_t read(atl_ep_t* ep,
                              void* buf,
                              size_t len,
                              atl_mr_t* mr,
                              uint64_t addr,
                              uintptr_t remote_key,
                              int dst_proc_idx,
                              atl_req_t* req) = 0;

    virtual atl_status_t write(atl_ep_t* ep,
                               const void* buf,
                               size_t len,
                               atl_mr_t* mr,
                               uint64_t addr,
                               uintptr_t remote_key,
                               int dst_proc_idx,
                               atl_req_t* req) = 0;

    virtual atl_status_t wait(atl_ep_t* ep, atl_req_t* req) = 0;

    virtual atl_status_t wait_all(atl_ep_t* ep, atl_req_t* req, size_t count) = 0;

    virtual atl_status_t cancel(atl_ep_t* ep, atl_req_t* req) = 0;

    virtual atl_status_t poll(atl_ep_t* ep) = 0;

    virtual atl_status_t check(atl_ep_t* ep, atl_req_t* req) = 0;
    virtual bool is_inited() = 0;
};
#endif