#ifndef ATL_H
#define ATL_H

#include <stddef.h>
#include <stdint.h>
#include <sys/uio.h>

// TODO: remove:
#define HAVE_OFI    1
#define HAVE_OFI_DL 0
#define HAVE_MPI    0
#define HAVE_MPI_DL 0

/* TODO move to utility code */
#ifndef container_of
#define container_of(ptr, type, field) \
	((type *) ((char *)ptr - offsetof(type, field)))
#endif

#define ATL_CACHELINE_LEN 64

#define ATL_REQ_SIZE 8

typedef enum {
    atl_status_success   = 0,
    atl_status_failure   = 1,
} atl_status_t;

typedef struct atl_desc atl_desc_t;
typedef struct atl_comm atl_comm_t;

typedef struct atl_ops {
    atl_status_t (*finalize)(atl_desc_t *desc, atl_comm_t **comms);
} atl_ops_t;

struct atl_desc {
    atl_ops_t *ops;
};

typedef struct atl_comm_attr {
} atl_comm_attr_t;

typedef struct atl_attr {
    size_t comm_count;
    atl_comm_attr_t comm_attr;
} atl_attr_t;

typedef struct atl_req {
    uint64_t tag;
    size_t remote_proc_idx;

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
} atl_pt2pt_ops_t;

typedef struct atl_comp_ops {
    atl_status_t (*wait)(atl_comm_t *comm, atl_req_t *req);
    atl_status_t (*wait_all)(atl_comm_t *comm, atl_req_t *reqs, size_t count);
    atl_status_t (*test)(atl_comm_t *comm, int *status, atl_req_t *req);
} atl_comp_ops_t;

struct atl_comm {
    atl_desc_t *atl_desc;
    atl_pt2pt_ops_t *pt2pt_ops;
    atl_comp_ops_t *comp_ops;
};

#define ARG_LIST                          \
    int *argc, char ***argv,              \
    size_t *proc_idx, size_t *proc_count, \
    atl_attr_t *attr,                     \
    atl_comm_t ***atl_comms,              \
    atl_desc_t **atl_desc

/*
 * Dynamically loaded providers must export the following entry point.
 * This is invoked by the ATL framework when the provider library
 * is loaded.
 */
#define ATL_EXT_INI \
__attribute__((visibility ("default"),EXTERNALLY_VISIBLE)) \
atl_status_t atl_ini(ARG_LIST)


/* Provider initialization function signature that built-in providers
 * must specify. */
#define INI_SIG(name)   \
atl_status_t name(ARG_LIST)

/* for each provider defines for three scenarios:
 * dl: externally visible ctor with known name (see fi_prov.h)
 * built-in: ctor function def, don't export symbols
 * not built: no-op call for ctor
 */

#if (HAVE_OFI) && (HAVE_OFI_DL)
#  define ATL_OFI_INI ATL_EXT_INI
#  define ATL_OFI_INIT atl_noop_init
#elif (HAVE_OFI)
#  define ATL_OFI_INI INI_SIG(atl_ofi_init)
#  define ATL_OFI_INIT atl_ofi_init
ATL_OFI_INI ;
#else
#  define ATL_OFI_INIT atl_noop_init
#endif

static inline INI_SIG(atl_noop_init)
{
    return 0;
}

atl_status_t atl_init(ARG_LIST);

static inline atl_status_t atl_finalize(atl_desc_t *desc, atl_comm_t **comms)
{
    return desc->ops->finalize(desc, comms);
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

static inline atl_status_t atl_comm_wait(atl_comm_t *comm, atl_req_t *req)
{
    return comm->comp_ops->wait(comm, req);
}

static inline atl_status_t atl_comm_wait_all(atl_comm_t *comm, atl_req_t *req, size_t count)
{
    return comm->comp_ops->wait_all(comm, req, count);
}

static inline atl_status_t atl_comm_test(atl_comm_t *comm, int *status, atl_req_t *req)
{
    return comm->comp_ops->test(comm, status, req);
}

#endif
