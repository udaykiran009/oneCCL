#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

// TODO: remove:
#define HAVE_OFI    1
#define HAVE_OFI_DL 1
#define HAVE_MPI    0
#define HAVE_MPI_DL 0

/* TODO move to utility code */
#ifndef container_of
#define container_of(ptr, type, field) \
	((type *) ((char *)ptr - offsetof(type, field)))
#endif

#define ATL_CACHELINE_LEN 64

#define ATL_REQ_SIZE 8

typedef enum framework_answers
{
    FA_WAIT     = 0,
    FA_USE      = 1,
    FA_FINALIZE = 2,
}framework_answers_t;

typedef framework_answers_t (*update_checker_f)(size_t comm_size);

typedef enum {
    atl_status_success   = 0,
    atl_status_failure   = 1,
} atl_status_t;

typedef struct atl_desc atl_desc_t;
typedef struct atl_comm atl_comm_t;

typedef struct atl_comm_attr {
} atl_comm_attr_t;

typedef struct atl_attr {
    size_t comm_count;
    int enable_rma;
    size_t max_order_waw_size;
    atl_comm_attr_t comm_attr;
} atl_attr_t;

typedef struct atl_mr {
    void *buf;
    size_t len;
    uintptr_t l_key;
    uintptr_t r_key;
} atl_mr_t;

typedef struct atl_ops {
    void (*proc_idx)(atl_desc_t *desc, size_t *proc_idx);
    void (*proc_count)(atl_desc_t *desc, size_t *proc_count);
    atl_status_t (*finalize)(atl_desc_t *desc, atl_comm_t **comms);
    atl_status_t (*update)(size_t *proc_idx, size_t *proc_count, atl_desc_t *desc);
    atl_status_t (*set_framework_function)(update_checker_f user_checker);
} atl_ops_t;

typedef struct atl_mr_ops {
    atl_status_t (*mr_reg)(atl_desc_t *atl_desc, const void *buf, size_t len,
                           atl_mr_t **atl_mr);
    atl_status_t (*mr_dereg)(atl_desc_t *atl_desc, atl_mr_t *atl_mr);
} atl_mr_ops_t;

struct atl_desc {
    atl_ops_t *ops;
    atl_mr_ops_t *mr_ops;
};

typedef struct atl_transport {
    const char *name;
    atl_status_t (*init)(int *argc, char ***argv, size_t *proc_idx, size_t *proc_count,
                         atl_attr_t *attr, atl_comm_t ***atl_comms, atl_desc_t **atl_desc);
} atl_transport_t;

typedef struct atl_req {
    uint64_t tag;
    size_t remote_proc_idx;
    size_t recv_len;

    void *internal[ATL_REQ_SIZE];
} atl_req_t __attribute__ ((aligned (ATL_CACHELINE_LEN)));

typedef struct atl_pt2pt_ops {
    /* Non-blocking I/O vector pt2pt ops */
    atl_status_t (*sendv)(atl_comm_t *comm, const struct iovec *iov, size_t count,
                          size_t dest_proc_idx, uint64_t tag, atl_req_t *req);
    atl_status_t (*recvv)(atl_comm_t *comm, struct iovec *iov, size_t count,
                          size_t src_proc_idx, uint64_t tag, atl_req_t *req);
    /* Non-blocking pt2pt ops */
    atl_status_t (*send)(atl_comm_t *comm, const void *buf, size_t len,
                         size_t dest_proc_idx, uint64_t tag, atl_req_t *req);
    atl_status_t (*recv)(atl_comm_t *comm, void *buf, size_t len,
                         size_t src_proc_idx, uint64_t tag, atl_req_t *req);
    atl_status_t (*probe)(atl_comm_t *comm, size_t src_proc_idx, uint64_t tag,
                          atl_req_t *req);
} atl_pt2pt_ops_t;

typedef struct atl_rma_ops {
    atl_status_t (*read)(atl_comm_t *comm, void *buf, size_t len, atl_mr_t *atl_mr,
                         uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req);
    atl_status_t (*write)(atl_comm_t *comm, const void *buf, size_t len, atl_mr_t *atl_mr,
       uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req);
} atl_rma_ops_t;

typedef struct atl_comp_ops {
    atl_status_t (*wait)(atl_comm_t *comm, atl_req_t *req);
    atl_status_t (*wait_all)(atl_comm_t *comm, atl_req_t *reqs, size_t count);
    atl_status_t (*poll)(atl_comm_t *comm);
    atl_status_t (*check)(atl_comm_t *comm, int *status, atl_req_t *req);
} atl_comp_ops_t;

struct atl_comm {
    atl_desc_t *atl_desc;
    atl_pt2pt_ops_t *pt2pt_ops;
    atl_rma_ops_t *rma_ops;
    atl_comp_ops_t *comp_ops;
};

/*
 * Dynamically loaded transports must export the following entry point.
 * This is invoked by the ATL framework when the transport library
 * is loaded.
 */
#define ATL_EXT_INI                                        \
__attribute__((visibility ("default"))) \
atl_status_t atl_ini(atl_transport_t *atl_transport)

/* Transport initialization function signature that built-in transportss
 * must specify. */
#define INI_SIG(name)                            \
atl_status_t name(atl_transport_t *atl_transport)

/* for each transport defines for three scenarios:
 * dl: externally visible ctor with known name
 * built-in: ctor function def, don't export symbols
 * not built: no-op call for ctor
 */

#if (HAVE_OFI) && (HAVE_OFI_DL)
#  define ATL_OFI_INI ATL_EXT_INI
#  define ATL_OFI_INIT atl_noop_init
#elif (HAVE_OFI)
#  define ATL_OFI_INI INI_SIG(atl_ofi_ini)
#  define ATL_OFI_INIT atl_ofi_init
ATL_OFI_INI ;
#else
#  define ATL_OFI_INIT atl_noop_init
#endif

static inline INI_SIG(atl_noop_init)
{
    return atl_status_success;
}

atl_status_t atl_init(int *argc, char ***argv, size_t *proc_idx, size_t *proc_count,
                      atl_attr_t *attr, atl_comm_t ***atl_comms, atl_desc_t **atl_desc);

static inline void atl_proc_idx(atl_desc_t *desc, size_t *proc_idx)
{
    return desc->ops->proc_idx(desc, proc_idx);
}

static inline void atl_proc_count(atl_desc_t *desc, size_t *proc_count)
{
    return desc->ops->proc_count(desc, proc_count);
}

static inline atl_status_t atl_update(size_t *proc_idx, size_t *proc_count, atl_desc_t *desc)
{
    return desc->ops->update(proc_idx, proc_count, desc);
}

static inline atl_status_t atl_finalize(atl_desc_t *desc, atl_comm_t **comms)
{
    return desc->ops->finalize(desc, comms);
}

static inline atl_status_t atl_set_framework_function(atl_desc_t *desc, update_checker_f user_checker)
{
    return desc->ops->set_framework_function(user_checker);
}

static inline atl_status_t atl_comm_send(atl_comm_t *comm, const void *buf, size_t len,
                                         size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    return comm->pt2pt_ops->send(comm, buf, len, dest_proc_idx, tag, req);
}

static inline atl_status_t atl_comm_recv(atl_comm_t *comm, void *buf, size_t len,
                                         size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    return comm->pt2pt_ops->recv(comm, buf, len, src_proc_idx, tag, req);
}

static inline atl_status_t atl_comm_sendv(atl_comm_t *comm, const struct iovec *iov, size_t count,
                                          size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    return comm->pt2pt_ops->sendv(comm, iov, count, dest_proc_idx, tag, req);
}

static inline atl_status_t atl_comm_recvv(atl_comm_t *comm, struct iovec *iov, size_t count,
                                          size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    return comm->pt2pt_ops->recvv(comm, iov, count, src_proc_idx, tag, req);
}

static inline atl_status_t atl_mr_reg(atl_desc_t *atl_desc, const void *buf, size_t len,
                                      atl_mr_t **atl_mr)
{
    return atl_desc->mr_ops->mr_reg(atl_desc, buf, len, atl_mr);
}

static inline atl_status_t atl_mr_dereg(atl_desc_t *atl_desc, atl_mr_t *atl_mr)
{
    return atl_desc->mr_ops->mr_dereg(atl_desc, atl_mr);
}

static inline atl_status_t atl_comm_read(atl_comm_t *comm, void *buf, size_t len, atl_mr_t *atl_mr,
                                         uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req)
{
    return comm->rma_ops->read(comm, buf, len, atl_mr, addr, r_key, dest_proc_idx, req);
}

static inline atl_status_t atl_comm_write(atl_comm_t *comm, const void *buf, size_t len, atl_mr_t *atl_mr,
                                          uint64_t addr, uintptr_t r_key, size_t dest_proc_idx, atl_req_t *req)
{
    return comm->rma_ops->write(comm, buf, len, atl_mr, addr, r_key, dest_proc_idx, req);
}

static inline atl_status_t atl_comm_probe(atl_comm_t *comm, size_t src_proc_idx,
                                          uint64_t tag, atl_req_t *req)
{
    return comm->pt2pt_ops->probe(comm, src_proc_idx, tag, req);
}

static inline atl_status_t atl_comm_wait(atl_comm_t *comm, atl_req_t *req)
{
    return comm->comp_ops->wait(comm, req);
}

static inline atl_status_t atl_comm_wait_all(atl_comm_t *comm, atl_req_t *req, size_t count)
{
    return comm->comp_ops->wait_all(comm, req, count);
}

static inline atl_status_t atl_comm_poll(atl_comm_t *comm)
{
    return comm->comp_ops->poll(comm);
}

static inline atl_status_t atl_comm_check(atl_comm_t *comm, int *status, atl_req_t *req)
{
    return comm->comp_ops->check(comm, status, req);
}
