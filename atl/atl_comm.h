#ifndef ATL_PT2PT_H
#define ATL_PT2PT_H

#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

#define ATL_REQ_SIZE 4

typedef struct atl_desc atl_desc_t;

typedef struct atl_req {
    void *internal[ATL_REQ_SIZE];
} atl_req_t;

typedef struct atl_comm atl_comm_t;

typedef enum atl_status atl_status_t;

typedef struct atl_pt2pt_ops {
    /* Non-blocking I/O vector pt2pt ops */
    atl_status_t (*sendv)(atl_comm_t *comm, const struct iovec *iov, size_t count,
                           uint64_t tag, atl_req_t *req);
    atl_status_t (*recvv)(atl_comm_t *comm, struct iovec *iov, size_t count,
                           uint64_t tag, atl_req_t *req);
    /* Non-blocking pt2pt ops */
    atl_status_t (*send)(atl_comm_t *comm, const void *buf, size_t len,
                           uint64_t tag, atl_req_t *req);
    atl_status_t (*recv)(atl_comm_t *comm, void *buf, size_t len,
                          uint64_t tag, atl_req_t *req);

    atl_status_t (*wait)(atl_comm_t *comm, atl_req_t *req);
    atl_status_t (*wait_all)(atl_comm_t *comm, atl_req_t *reqs, size_t count);

    atl_status_t (*test)(atl_comm_t *comm, int *status, atl_req_t *req);
} atl_pt2pt_ops_t;

struct atl_comm {
    atl_desc_t *atl_desc;
    atl_pt2pt_ops_t *pt2pt;
};

typedef struct atl_comm_attr {
} atl_comm_attr_t;

#endif
