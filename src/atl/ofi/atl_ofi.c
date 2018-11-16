#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <assert.h>
#include <math.h>

#include <sys/types.h>
#include <unistd.h>
#include <sys/syscall.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>

#include <inttypes.h>
#include "pm_rt.h"

#include "atl.h"

#ifndef gettid
#define gettid() syscall(SYS_gettid)
#endif

#ifdef ENABLE_DEBUG
#define ATL_OFI_DEBUG_PRINT(s, ...)                  \
    do {                                             \
        pid_t tid = gettid();                        \
        char hoststr[32];                            \
        gethostname(hoststr, sizeof(hoststr));       \
        fprintf(stdout, "(%d): %s: @ %s:%d:%s() " s, \
                tid, hoststr,                        \
                __FILE__, __LINE__,                  \
                __func__, ##__VA_ARGS__);            \
        fflush(stdout);                              \
    } while (0)
#else
#define ATL_OFI_DEBUG_PRINT(s, ...)
#endif

#define ATL_OFI_RETRY(func, comm, ret_val)                        \
    do {                                                          \
        ret_val = func;                                           \
        if (ret_val == FI_SUCCESS)                                \
            break;                                                \
        if (ret_val != -FI_EAGAIN) {                              \
            ATL_OFI_DEBUG_PRINT(#func "fails with ret %"PRId64"", \
                                ret_val);                         \
            assert(0);                                            \
            break;                                                \
        }                                                         \
        (void) atl_ofi_comm_poll(comm);                           \
    } while (ret_val == -FI_EAGAIN)

#define ATL_OFI_PM_KEY "atl-ofi"

#define ATL_OFI_CQ_BUNCH_SIZE (8)

/* OFI returns 0 or -errno */
#define RET2ATL(ret) ((ret) ? atl_status_failure : atl_status_success)

static const char *atl_ofi_name = "OFI";

typedef struct atl_ofi_context {
    atl_desc_t atl_desc;
    size_t proc_idx;
    size_t proc_count;
    pm_rt_desc_t *pm_rt;
    struct fi_info *prov;
    struct fid_fabric *fabric;
    struct fid_domain *domain;
    struct fid_av *av;
    /* This is used only in case of SEP supported */
    struct fid_ep *sep;
    int rx_ctx_bits;
    size_t comm_ref_count;
    struct {
        fi_addr_t *table;
        size_t ep_num; /* table[][0..ep_num], this is 1 for SEP */
        /* table[0..proc_count][] */
    } addr_table;
    size_t addr_len;
} atl_ofi_context_t;

typedef struct atl_ofi_comm_name {
    void *addr;
    size_t len;
} atl_ofi_comm_name_t;

typedef struct atl_ofi_comm_context {
    atl_comm_t atl_comm;
    size_t idx;
    struct fid_ep *tx_ctx;
    struct fid_ep *rx_ctx;
    struct fid_cq *cq;
    atl_ofi_comm_name_t *name;
} atl_ofi_comm_context_t;

typedef enum atl_ofi_comp_state {
    ATL_OFI_COMP_POSTED,
    ATL_OFI_COMP_PROBE_STARTED,
    ATL_OFI_COMP_PROBE_COMPLETED,
    ATL_OFI_COMP_COMPLETED,
} atl_ofi_comp_state_t;

typedef struct atl_ofi_req {
    struct fi_context ofi_context;

    atl_ofi_comm_context_t *comm;
    atl_ofi_comp_state_t comp_state;
} atl_ofi_req_t;

static inline fi_addr_t
atl_ofi_comm_atl_addr_2_fi_addr(atl_ofi_context_t *atl, size_t proc_idx, size_t ep_idx)
{
    if (atl->sep)
        return fi_rx_addr(*(atl->addr_table.table + ((atl->addr_table.ep_num * proc_idx))),
                          ep_idx, atl->rx_ctx_bits);
    else
        return *(atl->addr_table.table + ((atl->addr_table.ep_num * proc_idx) + ep_idx));
}

static atl_status_t
atl_ofi_comms_connect(atl_ofi_context_t *atl_ofi_context,
                      atl_comm_t **comms, size_t comm_count)
{
    int ret;
    size_t i, j;
    char *epnames_table;
    size_t epnames_table_len;

    atl_ofi_context->addr_table.ep_num =
        (atl_ofi_context->sep ? 1 : comm_count);

    /* Allocate OFI EP names table that will contain all published names */
    epnames_table_len = atl_ofi_context->addr_len *
                        atl_ofi_context->addr_table.ep_num *
                        atl_ofi_context->proc_count;
    epnames_table = (char *)calloc(1, epnames_table_len);
    if (!epnames_table)
        return atl_status_failure;

    for (i = 0; i < atl_ofi_context->addr_table.ep_num; i++) {
        atl_ofi_comm_context_t *comm_context =
            container_of(comms[i], atl_ofi_comm_context_t, atl_comm);
        ret = pmrt_kvs_put(atl_ofi_context->pm_rt, ATL_OFI_PM_KEY,
                           atl_ofi_context->proc_idx, i,
                           comm_context->name->addr,
                           comm_context->name->len);
        if (ret)
            goto err_ep_names;
    }

    pmrt_barrier(atl_ofi_context->pm_rt);

    /* Retrieve all the OFI EP names in order */
    for (i = 0; i < atl_ofi_context->proc_count; i++) {
        for (j = 0; j < atl_ofi_context->addr_table.ep_num; j++) {
            ret = pmrt_kvs_get(atl_ofi_context->pm_rt, ATL_OFI_PM_KEY, i, j,
                               epnames_table +
                               (/* iterate by comms(EPs) per process */ j +
                                /* iterate by processes */
                                (i * atl_ofi_context->addr_table.ep_num)) *
                               atl_ofi_context->addr_len,
                               atl_ofi_context->addr_len);
            if (ret)
                goto err_ep_names;
        }
    }

    atl_ofi_context->addr_table.table =
        calloc(1, atl_ofi_context->addr_table.ep_num *
                  atl_ofi_context->proc_count *
                  sizeof(fi_addr_t));
    if (!atl_ofi_context->addr_table.table)
        goto err_ep_names;

    /* Insert all the EP names into the AV */
    ret = fi_av_insert(atl_ofi_context->av, epnames_table,
                       atl_ofi_context->addr_table.ep_num *
                       atl_ofi_context->proc_count,
                       atl_ofi_context->addr_table.table, 0, NULL);
    if (ret != atl_ofi_context->proc_count) {
        ret = atl_status_failure;
        goto err_addr_table;
    } else {
        ret = atl_status_success;
    }

    /* Normal end of execution */
    free(epnames_table);
    return ret;

    /*Abnormal end of execution */
err_addr_table:
    free(atl_ofi_context->addr_table.table);
    atl_ofi_context->addr_table.ep_num = 0;
err_ep_names:
    free(epnames_table);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_comms_destroy_conns(atl_ofi_context_t *atl_ofi_context)
{
    int ret;

    ret = fi_av_remove(atl_ofi_context->av, atl_ofi_context->addr_table.table,
                       atl_ofi_context->addr_table.ep_num *
                       atl_ofi_context->proc_count, 0);
    if (ret)
        ATL_OFI_DEBUG_PRINT("AV remove failed (%d)\n", ret);

    free(atl_ofi_context->addr_table.table);
    atl_ofi_context->addr_table.ep_num = 0;

    return RET2ATL(ret);
}

static atl_status_t
atl_ofi_comm_get_epname(struct fid_ep *ep, atl_ofi_comm_name_t **name)
{
    int ret;

    *name = calloc(1, sizeof(*name));
    if (!*name)
        return atl_status_failure;

    ret = fi_getname(&ep->fid, (*name)->addr, &(*name)->len);
    if ((ret != -FI_ETOOSMALL) || ((*name)->len <= 0))
        (*name)->len = FI_NAME_MAX;

    (*name)->addr = calloc(1, (*name)->len);
    if (!(*name)->addr)
        goto err_addr;

    ret = fi_getname(&ep->fid, (*name)->addr, &(*name)->len);
    if (ret)
        goto err_getname;

    return atl_status_success;
err_getname:
    free((*name)->addr);
    (*name)->len = 0;
err_addr:
    free(*name);
    *name = NULL;
    return RET2ATL(ret);
}

static void atl_ofi_comms_set_epname(atl_comm_t **comms, size_t comm_count,
                                     atl_ofi_comm_name_t *name)
{
    atl_ofi_comm_context_t *comm_context;
    int i;

    for (i = 0; i < comm_count; i++) {
        comm_context = container_of(comms[i], atl_ofi_comm_context_t, atl_comm);
        comm_context->name = name;
    }
}

static void ofi_comm_ep_destroy(atl_ofi_context_t *atl_ofi_context,
                                atl_ofi_comm_context_t *atl_ofi_comm_context)
{
    if (atl_ofi_context->sep) {
        if (atl_ofi_comm_context->rx_ctx)
            fi_close(&atl_ofi_comm_context->rx_ctx->fid);
        if (atl_ofi_comm_context->tx_ctx)
            fi_close(&atl_ofi_comm_context->tx_ctx->fid);
    } else {
        if (atl_ofi_comm_context->rx_ctx && atl_ofi_comm_context->tx_ctx)
            fi_close(&atl_ofi_comm_context->rx_ctx->fid);
        atl_ofi_comm_context->rx_ctx = atl_ofi_comm_context->tx_ctx = NULL;
    }
}

static void atl_ofi_comm_conn_destroy(atl_ofi_context_t *atl_ofi_context,
                                 atl_comm_t *comm)
{
    atl_ofi_comm_context_t *atl_ofi_comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);

    ofi_comm_ep_destroy(atl_ofi_context, atl_ofi_comm_context);
    if (atl_ofi_comm_context->name) {
        free(atl_ofi_comm_context->name->addr);
        atl_ofi_comm_context->name->addr = NULL;
        atl_ofi_comm_context->name->len = 0;
        free(atl_ofi_comm_context->name);
        atl_ofi_comm_context->name = NULL;
    }
}

static void atl_ofi_comms_conn_destroy(atl_ofi_context_t *atl_ofi_context,
                                       atl_comm_t **comms)
{
    size_t i;

    if (atl_ofi_context->sep) {
        atl_ofi_comm_context_t *atl_ofi_comm_context =
            container_of(comms[0], atl_ofi_comm_context_t, atl_comm);
        /* Free memory for one of the COMM contexts, for other - just set to NULL*/
        free(atl_ofi_comm_context->name->addr);
        atl_ofi_comm_context->name->addr = NULL;
        atl_ofi_comm_context->name->len = 0;
        free(atl_ofi_comm_context->name);
        atl_ofi_comms_set_epname(comms, atl_ofi_context->comm_ref_count, NULL);
    }

    for (i = 0; i < atl_ofi_context->comm_ref_count; i++)
        atl_ofi_comm_conn_destroy(atl_ofi_context, comms[i]);

    if (atl_ofi_context->sep)
        fi_close(&atl_ofi_context->sep->fid);
}

static atl_status_t atl_ofi_finalize(atl_desc_t *atl_desc, atl_comm_t **atl_comms)
{
    int ret;
    size_t i;
    atl_ofi_context_t *atl_ofi_context =
        container_of(atl_desc, atl_ofi_context_t, atl_desc);

    ret = atl_ofi_comms_destroy_conns(atl_ofi_context);
    atl_ofi_comms_conn_destroy(atl_ofi_context, atl_comms);
    for (i = 0; i < atl_ofi_context->comm_ref_count; i++) {
        atl_ofi_comm_context_t *atl_ofi_comm_context =
            container_of(atl_comms[i], atl_ofi_comm_context_t, atl_comm);
        if (atl_ofi_comm_context->cq)
            fi_close(&atl_ofi_comm_context->cq->fid);
        free(atl_ofi_comm_context);
    }
    free(atl_comms);
    fi_close(&atl_ofi_context->av->fid);
    fi_close(&atl_ofi_context->domain->fid);
    fi_close(&atl_ofi_context->fabric->fid);
    fi_freeinfo(atl_ofi_context->prov);
    pmrt_finalize(atl_ofi_context->pm_rt);
    free(atl_ofi_context);

    return RET2ATL(ret);
}

static atl_status_t
ofi_comm_ep_create(atl_ofi_context_t *atl_ofi_context,
                   atl_ofi_comm_context_t *atl_ofi_comm_context,
                   size_t index)
{
    int ret;
    struct fi_cq_attr cq_attr = {
        .format = FI_CQ_FORMAT_TAGGED,
    };
    struct fi_tx_attr tx_attr;
    struct fi_rx_attr rx_attr;

    ret = fi_cq_open(atl_ofi_context->domain, &cq_attr,
                     &atl_ofi_comm_context->cq, NULL);
    if (ret)
        return atl_status_failure;

    if (atl_ofi_context->sep) {
        rx_attr = *atl_ofi_context->prov->rx_attr;
        rx_attr.caps |= FI_TAGGED | FI_RECV;
        ret = fi_rx_context(atl_ofi_context->sep, index, &rx_attr,
                            &atl_ofi_comm_context->rx_ctx, NULL);
        if (ret)
            goto err;
        ret = fi_ep_bind(atl_ofi_comm_context->rx_ctx,
                         &atl_ofi_comm_context->cq->fid, FI_RECV);
        if (ret)
            goto err;

        tx_attr = *atl_ofi_context->prov->tx_attr;
        tx_attr.caps |= FI_TAGGED | FI_SEND;
        ret = fi_tx_context(atl_ofi_context->sep, index, &tx_attr,
                            &atl_ofi_comm_context->tx_ctx, NULL);
        if (ret)
            goto err;
        ret = fi_ep_bind(atl_ofi_comm_context->tx_ctx,
                         &atl_ofi_comm_context->cq->fid, FI_SEND);
        if (ret)
            goto err;

        fi_enable(atl_ofi_comm_context->rx_ctx);
        fi_enable(atl_ofi_comm_context->tx_ctx);
    } else {
        struct fid_ep *ep;

        ret = fi_endpoint(atl_ofi_context->domain, atl_ofi_context->prov, &ep, NULL);
        if (ret)
            goto err;
        atl_ofi_comm_context->tx_ctx = atl_ofi_comm_context->rx_ctx = ep;
        ret = fi_ep_bind(ep, &atl_ofi_comm_context->cq->fid, FI_SEND | FI_RECV);
        if (ret)
            goto err;
        ret = fi_ep_bind(ep, &atl_ofi_context->av->fid, 0);
        if (ret)
            goto err;

        fi_enable(ep);

        ret = atl_ofi_comm_get_epname(ep, &atl_ofi_comm_context->name);
        if (ret)
            goto err;
        atl_ofi_context->addr_len = atl_ofi_comm_context->name->len;
        if (ret)
            goto err;
    }

    return atl_status_success;
err:
    ofi_comm_ep_destroy(atl_ofi_context, atl_ofi_comm_context);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_comm_handle_cq_err(atl_ofi_comm_context_t *comm_context)
{
    struct fi_cq_err_entry err_entry;
    int ret = fi_cq_readerr(comm_context->cq, &err_entry, 0);
    if (ret != 1) {
        ATL_OFI_DEBUG_PRINT("Unable to fi_cq_readerr\n");
        assert(0);
        return atl_status_failure;
    } else {
        if (err_entry.err != FI_ENOMSG) {
            ATL_OFI_DEBUG_PRINT("fi_cq_readerr: err: %d, prov_err: %s (%d)\n",
                                err_entry.err, fi_cq_strerror(comm_context->cq,
                                                              err_entry.prov_errno,
                                                              err_entry.err_data,
                                                              NULL, 0),
                                err_entry.prov_errno);
            return atl_status_failure;
        }
        return atl_status_success;
    }
}

static inline void atl_ofi_process_comps(struct fi_cq_tagged_entry *entries,
                                         ssize_t ret)
{
    size_t idx;
    atl_ofi_req_t *comp_ofi_req;
    atl_req_t *comp_atl_req;

    for (idx = 0; idx < ret; idx++) {
        comp_ofi_req = container_of(entries[idx].op_context, atl_ofi_req_t,
                                    ofi_context);
        switch (comp_ofi_req->comp_state) {
        case ATL_OFI_COMP_POSTED:
            comp_ofi_req->comp_state = ATL_OFI_COMP_COMPLETED;
            break;
        case ATL_OFI_COMP_PROBE_STARTED:
            comp_ofi_req->comp_state = ATL_OFI_COMP_PROBE_COMPLETED;
            break;
        default:
            assert(0);
            break;
        }
        if (entries[idx].flags & FI_RECV) {
            comp_atl_req = container_of(comp_ofi_req, atl_req_t, internal);
            comp_atl_req->recv_len = entries[idx].len;
        }
    }
}

static inline atl_status_t atl_ofi_comm_poll(atl_comm_t *comm)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    struct fi_cq_tagged_entry entries[ATL_OFI_CQ_BUNCH_SIZE];
    ssize_t ret;

    do {
        ret = fi_cq_read(comm_context->cq, entries, ATL_OFI_CQ_BUNCH_SIZE);
        if (ret > 0) {
            atl_ofi_process_comps(entries, ret);
        } else if (ret == -FI_EAGAIN) {
            break;
        } else {
            return atl_ofi_comm_handle_cq_err(comm_context);
        }
    } while (ret > 0);

    return atl_status_success;
}

static atl_status_t
atl_ofi_comm_handle_probe_comp(atl_comm_t *comm,
                               struct iovec *iov, size_t count,
                               atl_req_t *req)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;
    struct fi_msg_tagged msg = {
        .msg_iov = iov,
        .desc = NULL,
        .iov_count = count,
        .addr = atl_ofi_comm_atl_addr_2_fi_addr(
                    container_of(comm->atl_desc,
                                 atl_ofi_context_t,
                                 atl_desc),
                    req->remote_proc_idx, comm_context->idx),
        .tag = req->tag,
        .ignore = 0,
        .context = &ofi_req->ofi_context,
        .data = 0,
    };

    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    (void) atl_ofi_comm_poll(comm);

    ATL_OFI_RETRY(fi_trecvmsg(comm_context->rx_ctx, &msg,
                              FI_CLAIM),
                  comm, ret);
    return RET2ATL(ret);
}

/* Non-blocking I/O vector pt2pt ops */
static atl_status_t
atl_ofi_comm_sendv(atl_comm_t *comm, const struct iovec *iov, size_t count,
                   size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;

    req->remote_proc_idx = dest_proc_idx;
    req->tag = tag;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ATL_OFI_RETRY(fi_tsendv(comm_context->tx_ctx, iov, NULL, count,
                             atl_ofi_comm_atl_addr_2_fi_addr(
                             container_of(comm->atl_desc,
                                          atl_ofi_context_t,
                                          atl_desc),
                             dest_proc_idx, comm_context->idx),
                             tag, &ofi_req->ofi_context),
                  comm, ret);
    return RET2ATL(ret);
}

static atl_status_t
atl_ofi_comm_recvv(atl_comm_t *comm, struct iovec *iov, size_t count,
                   size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;

    if (ofi_req->comp_state == ATL_OFI_COMP_PROBE_COMPLETED) {
        return atl_ofi_comm_handle_probe_comp(comm, iov, count, req);
    }

    req->remote_proc_idx = src_proc_idx;
    req->tag = tag;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ATL_OFI_RETRY(fi_trecvv(comm_context->rx_ctx, iov, NULL, count,
                            atl_ofi_comm_atl_addr_2_fi_addr(
                                 container_of(comm->atl_desc,
                                              atl_ofi_context_t,
                                              atl_desc),
                                 src_proc_idx, comm_context->idx),
                            tag, 0, &ofi_req->ofi_context),
                  comm, ret);
    return RET2ATL(ret);
}

/* Non-blocking pt2pt ops */
static atl_status_t
atl_ofi_comm_send(atl_comm_t *comm, const void *buf, size_t len,
                  size_t dest_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;

    req->remote_proc_idx = dest_proc_idx;
    req->tag = tag;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ATL_OFI_RETRY(fi_tsend(comm_context->tx_ctx, buf, len, NULL,
                           atl_ofi_comm_atl_addr_2_fi_addr(
                                container_of(comm->atl_desc,
                                             atl_ofi_context_t,
                                             atl_desc),
                                dest_proc_idx, comm_context->idx),
                           tag, &ofi_req->ofi_context),
                  comm, ret);
    return RET2ATL(ret);
}

static atl_status_t
atl_ofi_comm_recv(atl_comm_t *comm, void *buf, size_t len,
                  size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;

    if (ofi_req->comp_state == ATL_OFI_COMP_PROBE_COMPLETED) {
        struct iovec iov = {
            .iov_base = buf,
            .iov_len = len,
        };
        return atl_ofi_comm_handle_probe_comp(comm, &iov, 1, req);
    }

    req->remote_proc_idx = src_proc_idx;
    req->tag = tag;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ATL_OFI_RETRY(fi_trecv(comm_context->rx_ctx, buf, len, NULL,
                           atl_ofi_comm_atl_addr_2_fi_addr(
                                container_of(comm->atl_desc,
                                             atl_ofi_context_t,
                                             atl_desc),
                                src_proc_idx, comm_context->idx),
                           tag, 0, &ofi_req->ofi_context),
                  comm, ret);
    return RET2ATL(ret);
}

static atl_status_t
atl_ofi_comm_probe(atl_comm_t *comm, size_t src_proc_idx, uint64_t tag, atl_req_t *req)
{
    
    atl_ofi_comm_context_t *comm_context =
        container_of(comm, atl_ofi_comm_context_t, atl_comm);
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    ssize_t ret;
    struct fi_msg_tagged msg = {
        .msg_iov = NULL,
        .desc = NULL,
        .iov_count = 0,
        .addr = atl_ofi_comm_atl_addr_2_fi_addr(
                    container_of(comm->atl_desc,
                                 atl_ofi_context_t,
                                 atl_desc),
                    src_proc_idx, comm_context->idx),
        .tag = tag,
        .ignore = 0,
        .context = &ofi_req->ofi_context,
        .data = 0,
    };

    req->remote_proc_idx = src_proc_idx;
    req->tag = tag;
    ofi_req->comp_state = ATL_OFI_COMP_PROBE_STARTED;

    (void) atl_ofi_comm_poll(comm);

    ATL_OFI_RETRY(fi_trecvmsg(comm_context->rx_ctx, &msg,
                              FI_PEEK | FI_COMPLETION),
                  comm, ret);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_comm_wait(atl_comm_t *comm, atl_req_t *req)
{
    atl_status_t ret = atl_status_success;
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);

    while (((ofi_req->comp_state != ATL_OFI_COMP_COMPLETED) &&
	    (ofi_req->comp_state != ATL_OFI_COMP_PROBE_COMPLETED)) &&
           ((ret = atl_ofi_comm_poll(comm)) == atl_status_success));

    return ret;
}

static atl_status_t
atl_ofi_comm_wait_all(atl_comm_t *comm, atl_req_t *reqs, size_t count)
{
    size_t i;
    atl_status_t ret;

    for (i = 0; i < count; i++) {
        ret = atl_ofi_comm_wait(comm, &reqs[i]);
        if (ret != atl_status_success)
            return ret;
    }

    return atl_status_success;
}

static atl_status_t
atl_ofi_comm_check(atl_comm_t *comm, int *status, atl_req_t *req)
{
    atl_ofi_req_t *ofi_req = ((atl_ofi_req_t *)req->internal);
    *status = ((ofi_req->comp_state == ATL_OFI_COMP_COMPLETED) ||
               (ofi_req->comp_state == ATL_OFI_COMP_PROBE_COMPLETED));
    return atl_status_success;
}

static atl_pt2pt_ops_t atl_ofi_comm_pt2pt_ops = {
    .sendv = atl_ofi_comm_sendv,
    .recvv = atl_ofi_comm_recvv,
    .send = atl_ofi_comm_send,
    .recv = atl_ofi_comm_recv,
    .probe = atl_ofi_comm_probe,
};

static atl_comp_ops_t atl_ofi_comm_comp_ops = {
    .wait = atl_ofi_comm_wait,
    .wait_all = atl_ofi_comm_wait_all,
    .poll = atl_ofi_comm_poll,
    .check = atl_ofi_comm_check
};

static atl_status_t
atl_ofi_comm_init(atl_ofi_context_t *atl_ofi_context, atl_comm_attr_t *attr,
                  size_t index, atl_comm_t **comm)
{
    int ret;
    atl_ofi_comm_context_t *atl_ofi_comm_context;

    atl_ofi_comm_context = calloc(1, sizeof(*atl_ofi_comm_context));
    if (!atl_ofi_context)
        return atl_status_failure;

    ret = ofi_comm_ep_create(atl_ofi_context, atl_ofi_comm_context, index);
    if (ret)
        goto err_ep;

    atl_ofi_comm_context->idx = index;
    *comm = &atl_ofi_comm_context->atl_comm;
    (*comm)->atl_desc = &atl_ofi_context->atl_desc;
    (*comm)->pt2pt_ops = &atl_ofi_comm_pt2pt_ops;
    (*comm)->comp_ops = &atl_ofi_comm_comp_ops;

    return atl_status_success;
err_ep:
    free(atl_ofi_comm_context);
    return RET2ATL(ret);
}

static atl_status_t
atl_ofi_comms_init(atl_ofi_context_t *atl_ofi_context, size_t comm_count,
                   atl_comm_attr_t *attr, atl_comm_t ***comms)
{
    int i = 0, ret;

    *comms = calloc(1, sizeof(**comms) * comm_count);
    if (!*comms)
        return atl_status_failure;

    if (atl_ofi_context->prov->domain_attr->max_ep_tx_ctx > 1) {
        ret = fi_scalable_ep(atl_ofi_context->domain, atl_ofi_context->prov,
                             &atl_ofi_context->sep, NULL);
        if (ret)
            goto err_sep;
        ret = fi_scalable_ep_bind(atl_ofi_context->sep,
                                  &atl_ofi_context->av->fid, 0);
        if (ret) {
            fi_close(&atl_ofi_context->sep->fid);
            goto err_sep;
        }
    }

    for (i = 0; i < comm_count; i++) {
        ret = atl_ofi_comm_init(atl_ofi_context, attr, i, &(*comms)[i]);
        if (ret)
            goto err_comm;
        atl_ofi_context->comm_ref_count++;
    }

    if (atl_ofi_context->sep) {
        atl_ofi_comm_name_t *name;

        fi_enable(atl_ofi_context->sep);

        atl_ofi_comm_get_epname(atl_ofi_context->sep, &name);
        atl_ofi_comms_set_epname(*comms, comm_count, name);
        atl_ofi_context->addr_len = name->len;
    }

    ret = atl_ofi_comms_connect(atl_ofi_context, *comms, comm_count);
    if (ret)
        goto err_comm;

    return atl_status_success;
err_comm:
    if (atl_ofi_context->sep)
        fi_close(&atl_ofi_context->sep->fid);
    while (atl_ofi_context->comm_ref_count) {
        atl_ofi_comm_context_t *atl_ofi_comm_context =
            container_of((*comms)[atl_ofi_context->comm_ref_count],
                         atl_ofi_comm_context_t, atl_comm);
        atl_ofi_comm_conn_destroy(atl_ofi_context,
                                  (*comms)[atl_ofi_context->comm_ref_count]);
        free(atl_ofi_comm_context);
        atl_ofi_context->comm_ref_count--;
    }
err_sep:
    free(*comms);
    return RET2ATL(ret);
}

static void atl_ofi_proc_idx(atl_desc_t *atl_desc, size_t *proc_idx)
{
    atl_ofi_context_t *atl_ofi_context =
        container_of(atl_desc, atl_ofi_context_t, atl_desc);
    *proc_idx = atl_ofi_context->proc_idx;
}

static void atl_ofi_proc_count(atl_desc_t *atl_desc, size_t *proc_count)
{
    atl_ofi_context_t *atl_ofi_context =
        container_of(atl_desc, atl_ofi_context_t, atl_desc);
    *proc_count = atl_ofi_context->proc_count;
}

atl_ops_t atl_ofi_ops = {
    .proc_idx = atl_ofi_proc_idx,
    .proc_count = atl_ofi_proc_count,
    .finalize = atl_ofi_finalize,
};

static void atl_ofi_tune(void)
{
    setenv("FI_PSM2_LOCK_LEVEL", "0", 0);
    setenv("HFI_NO_CPUAFFINITY", "1", 0);
}

atl_status_t atl_ofi_init(int *argc, char ***argv, size_t *proc_idx, size_t *proc_count,
                          atl_attr_t *attr, atl_comm_t ***atl_comms, atl_desc_t **atl_desc)
{
    struct fi_info *providers, *hints;
    struct fi_av_attr av_attr = { (enum fi_av_type)0 };
    int fi_version, ret;
    atl_ofi_context_t *atl_ofi_context;

    assert(sizeof(atl_ofi_req_t) <= sizeof(atl_req_t) - offsetof(atl_req_t, internal));

    atl_ofi_tune();

    atl_ofi_context = calloc(1, sizeof(*atl_ofi_context));
    if (!atl_ofi_context)
        return atl_status_failure;

    ret = pmrt_init(&atl_ofi_context->proc_idx, &atl_ofi_context->proc_count,
                    &atl_ofi_context->pm_rt);
    if (ret)
        goto err_pmrt_init;

    *proc_idx = atl_ofi_context->proc_idx;
    *proc_count = atl_ofi_context->proc_count;

    hints = fi_allocinfo();
    if (!hints)
        goto err_hints;
    hints->mode = FI_CONTEXT;
    hints->ep_attr->type = FI_EP_RDM;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->caps = FI_TAGGED;
    hints->ep_attr->tx_ctx_cnt = attr->comm_count;
    hints->ep_attr->rx_ctx_cnt = attr->comm_count;

    fi_version = FI_VERSION(1, 0);

    ret = fi_getinfo(fi_version, NULL, NULL, 0ULL, hints, &providers);
    if (ret || !providers)
        goto err_getinfo;

    /* Use first provider from the list of providers */
    atl_ofi_context->prov = fi_dupinfo(providers);
    if (!atl_ofi_context->prov)
        goto err_prov;

    ret = fi_fabric(atl_ofi_context->prov->fabric_attr,
                    &atl_ofi_context->fabric, NULL);
    if (ret)
        goto err_fab;

    ret = fi_domain(atl_ofi_context->fabric, atl_ofi_context->prov,
                    &atl_ofi_context->domain, NULL);
    if (ret)
        goto err_dom;

    av_attr.type = FI_AV_TABLE;
    av_attr.rx_ctx_bits = atl_ofi_context->rx_ctx_bits =
        (int)ceil(log2(hints->ep_attr->rx_ctx_cnt));
    ret = fi_av_open(atl_ofi_context->domain, &av_attr,
                     &atl_ofi_context->av, NULL);
    if (ret)
        goto err_av;

    atl_ofi_context->comm_ref_count = 0;

    ret = atl_ofi_comms_init(atl_ofi_context, attr->comm_count,
                             &attr->comm_attr, atl_comms);
    if (ret)
        goto err_comms_init;

    atl_ofi_context->atl_desc.ops = &atl_ofi_ops;
    *atl_desc = &atl_ofi_context->atl_desc;

    return atl_status_success;
err_comms_init:
    fi_close(&atl_ofi_context->av->fid);
err_av:
    fi_close(&atl_ofi_context->domain->fid);
err_dom:
    fi_close(&atl_ofi_context->fabric->fid);
err_fab:
    fi_freeinfo(atl_ofi_context->prov);
err_prov:
    fi_freeinfo(providers);
err_getinfo:
    fi_freeinfo(hints);
err_hints:
    pmrt_finalize(atl_ofi_context->pm_rt);
err_pmrt_init:
    free(atl_ofi_context);
    return RET2ATL(ret);
}

ATL_OFI_INI
{
    atl_transport->name = atl_ofi_name;
    atl_transport->init = atl_ofi_init;

    return atl_status_success;
}
