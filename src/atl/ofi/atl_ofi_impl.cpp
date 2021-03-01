#include <assert.h>
#include <inttypes.h>
#include <math.h>
#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <sys/syscall.h>
#include <time.h>
#include <unistd.h>
#include <errno.h>

#include "atl.h"

#define ATL_OFI_BASE_PM_KEY         "atl-ofi"
#define ATL_OFI_FI_ADDR_PM_KEY      ATL_OFI_BASE_PM_KEY "-fiaddr"
#define ATL_OFI_HOSTNAME_PM_KEY     ATL_OFI_BASE_PM_KEY "-hostname"
#define ATL_OFI_TIMEOUT_SEC_ENV     "ATL_OFI_TIMEOUT_SEC"
#define ATL_OFI_MAX_RETRY_COUNT_ENV "ATL_OFI_MAX_RETRY_COUNT"
#define ATL_OFI_DEFAULT_TIMEOUT_SEC 60
#define ATL_OFI_MAX_RETRY_COUNT     10000
#define ATL_OFI_MAX_HOSTNAME_LEN    64
#define ATL_OFI_WAIT_SEC            10
#define ATL_OFI_CQ_READ_ITERS       10000
#define ATL_OFI_CQ_BUNCH_SIZE       8
#define ATL_OFI_MAX_PROV_ENV_LEN    128
#define ATL_OFI_PMI_PROV_MULTIPLIER 100
#define ATL_OFI_PMI_PROC_MULTIPLIER (ATL_OFI_PMI_PROV_MULTIPLIER * 10)
#define ATL_OFI_MAX_PROV_COUNT      2 /* NW and SHM providers */
#define ATL_OFI_SHM_PROV_NAME       "shm"

#ifndef PRId64
#define PRId64 "lld"
#endif

#define MAX(a, b) \
    ({ \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a > _b ? _a : _b; \
    })

#define MIN(a, b) \
    ({ \
        __typeof__(a) _a = (a); \
        __typeof__(b) _b = (b); \
        _a < _b ? _a : _b; \
    })

static inline atl_status_t atl_ofi_ep_poll(atl_ep_t* ep);

#define ATL_OFI_CALL(func, ret_val, err_action) \
    do { \
        ret_val = func; \
        if (ret_val != FI_SUCCESS) { \
            LOG_ERROR(#func "\n fails with ret: ", ret_val, ", strerror:", fi_strerror(-ret_val)); \
            err_action; \
        } \
    } while (0)

#define ATL_OFI_RETRY(func, ep, ret_val) \
    do { \
        atl_ctx_t* ctx = ep->ctx; \
        atl_ofi_ctx_t* ofi_ctx = container_of(ctx, atl_ofi_ctx_t, ctx); \
        size_t max_retry_count = ofi_ctx->max_retry_count; \
        size_t retry_count = 0; \
        do { \
            ret_val = func; \
            if (ret_val == FI_SUCCESS) \
                break; \
            if (ret_val != -FI_EAGAIN) { \
                LOG_ERROR( \
                    #func "\n fails with ret: ", ret_val, ", strerror: ", fi_strerror(-ret_val)); \
                CCL_THROW("OFI function error"); \
                break; \
            } \
            (void)atl_ofi_ep_poll(ep); \
            retry_count++; \
        } while ((ret_val == -FI_EAGAIN) && (retry_count < max_retry_count)); \
    } while (0)

/* OFI returns 0 or -errno */
#define RET2ATL(ret) \
    ({ \
        atl_status_t res; \
        if (ret == -FI_EAGAIN) \
            res = ATL_STATUS_AGAIN; \
        else \
            res = (ret) ? ATL_STATUS_FAILURE : ATL_STATUS_SUCCESS; \
        res; \
    })

long int safe_c_strtol(const char* str, char** endptr, int base) {
    long int val = strtol(str, endptr, base);
    if (val == 0) {
        /* if a conversion error occurred, display a message and exit */
        if (errno == EINVAL) {
            LOG_ERROR("conversion error occurred for string: ", str);
        }
        /* if the value provided was out of range, display a error message */
        if (errno == ERANGE) {
            LOG_ERROR("the value provided was out of range, string: ", str);
        }
    }
    return val;
}

typedef enum {
    ATL_OFI_COMP_POSTED,
    ATL_OFI_COMP_COMPLETED,
    ATL_OFI_COMP_PEEK_STARTED,
    ATL_OFI_COMP_PEEK_FOUND,
    ATL_OFI_COMP_PEEK_NOT_FOUND,
} atl_ofi_comp_state_t;

typedef struct {
    atl_mr_t mr;
    struct fid_mr* fi_mr;
} atl_ofi_mr_t;

typedef struct {
    void* addr;
    size_t len;
} atl_ofi_prov_ep_name_t;

typedef struct {
    struct fid_ep* tx;
    struct fid_ep* rx;
    struct fid_cq* cq;
    atl_ofi_prov_ep_name_t name;
} atl_ofi_prov_ep_t;

typedef struct {
    struct fi_info* info;
    struct fid_fabric* fabric;
    struct fid_domain* domain;
    struct fid_av* av;
    atl_ofi_prov_ep_t* eps;

    int is_shm;
    size_t max_msg_size;

    /* used only in case of SEP supported */
    struct fid_ep* sep;
    int rx_ctx_bits;

    /* table[0..proc_count][0..ep_count] */
    fi_addr_t* addr_table;
    size_t addr_len;
    int first_proc_idx;

} atl_ofi_prov_t;

typedef struct {
    atl_ep_t ep;
} atl_ofi_ep_t;

typedef struct {
    atl_ctx_t ctx;
    pm_rt_desc_t* pm_rt;
    atl_ofi_prov_t provs[ATL_OFI_MAX_PROV_COUNT];
    size_t prov_count;
    size_t shm_prov_idx;
    size_t nw_prov_idx;
    size_t max_retry_count;
    atl_progress_mode_t progress_mode;
} atl_ofi_ctx_t;

typedef struct {
    struct fi_context fi_ctx;
    atl_ofi_prov_ep_t* prov_ep;
    struct fid_ep* fi_ep;
    atl_ofi_comp_state_t comp_state;
    size_t recv_len;
} atl_ofi_req_t;

typedef struct atl_ofi_global_data {
    size_t ctx_count;
    int is_env_inited;
    char prov_env_copy[ATL_OFI_MAX_PROV_ENV_LEN];

    atl_ofi_global_data() : ctx_count(0), is_env_inited(0) {
        memset(prov_env_copy, 0, sizeof(prov_env_copy));
    }
} atl_ofi_global_data_t;

static atl_ofi_global_data_t global_data;

static void atl_ofi_print_coord(atl_proc_coord_t* coord) {
    LOG_DEBUG("coord: global [idx ",
              coord->global_idx,
              ", cnt ",
              coord->global_count,
              "], local [idx ",
              coord->local_idx,
              ", cnt ",
              coord->local_count,
              "]");
}

static inline atl_ofi_prov_t* atl_ofi_get_prov(atl_ep_t* ep, int peer_proc_idx, size_t msg_size) {
    size_t prov_idx;
    atl_ofi_ctx_t* ofi_ctx = container_of(ep->ctx, atl_ofi_ctx_t, ctx);

    if (ofi_ctx->prov_count == 1) {
        prov_idx = 0;
    }
    else {
        CCL_THROW_IF_NOT(ofi_ctx->prov_count == ATL_OFI_MAX_PROV_COUNT,
                         "unexpected prov_count ",
                         ofi_ctx->prov_count);

        atl_proc_coord_t* coord = &(ep->ctx->coord);
        int my_node_idx = coord->global_idx / coord->local_count;
        int peer_node_idx = peer_proc_idx / coord->local_count;

        if ((my_node_idx == peer_node_idx) &&
            (msg_size <= ofi_ctx->provs[ofi_ctx->shm_prov_idx].max_msg_size))
            prov_idx = ofi_ctx->shm_prov_idx;
        else
            prov_idx = ofi_ctx->nw_prov_idx;
    }

    /* TODO: add segmentation logic */
    CCL_THROW_IF_NOT(msg_size <= ofi_ctx->provs[prov_idx].max_msg_size,
                     "msg_size (",
                     msg_size,
                     ") is greater than max_msg_size (",
                     ofi_ctx->provs[prov_idx].max_msg_size,
                     "), prov_idx ",
                     prov_idx);

    return &(ofi_ctx->provs[prov_idx]);
}

static inline fi_addr_t atl_ofi_get_addr(atl_ctx_t* ctx,
                                         atl_ofi_prov_t* prov,
                                         int proc_idx,
                                         size_t ep_idx) {
    return *(prov->addr_table + ((ctx->ep_count * (proc_idx - prov->first_proc_idx)) + ep_idx));
}

static atl_status_t atl_ofi_get_local_proc_coord(atl_ofi_ctx_t* ofi_ctx, ipmi* pmi) {
    CCL_THROW_IF_NOT(ofi_ctx, "ofi_ctx is null");

    atl_proc_coord_t* coord = &(ofi_ctx->ctx.coord);

    atl_status_t ret = ATL_STATUS_SUCCESS;
    int i;
    int local_idx = 0, local_count = 0;
    char* all_hostnames = NULL;
    char my_hostname[ATL_OFI_MAX_HOSTNAME_LEN] = { 0 };
    size_t my_hostname_len = 0;
    int my_global_proc_idx = coord->global_idx;

    gethostname(my_hostname, ATL_OFI_MAX_HOSTNAME_LEN - 1);
    my_hostname_len = strlen(my_hostname);

    CCL_THROW_IF_NOT(my_hostname_len < ATL_OFI_MAX_HOSTNAME_LEN,
                     "unexpected my_hostname_len ",
                     my_hostname_len,
                     ", expected max ",
                     (size_t)(ATL_OFI_MAX_HOSTNAME_LEN));

    if (ATL_OFI_MAX_HOSTNAME_LEN - my_hostname_len <= 10) {
        LOG_WARN("hostname is quite long, len: ", my_hostname_len, ", name: ", my_hostname);
    }

    snprintf(my_hostname + my_hostname_len,
             ATL_OFI_MAX_HOSTNAME_LEN - my_hostname_len,
             "-%d",
             my_global_proc_idx);

    ret = pmi->pmrt_kvs_put((char*)ATL_OFI_HOSTNAME_PM_KEY,
                            my_global_proc_idx * ATL_OFI_PMI_PROC_MULTIPLIER,
                            my_hostname,
                            ATL_OFI_MAX_HOSTNAME_LEN);

    if (ret) {
        LOG_ERROR("pmrt_kvs_put: ret: ", ret);
        goto fn_err;
    }

    pmi->pmrt_barrier();

    all_hostnames = (char*)calloc(1, coord->global_count * ATL_OFI_MAX_HOSTNAME_LEN);
    if (!all_hostnames) {
        LOG_ERROR("can't allocate all_hostnames");
        goto fn_err;
    }

    for (i = 0; i < coord->global_count; i++) {
        ret = pmi->pmrt_kvs_get((char*)ATL_OFI_HOSTNAME_PM_KEY,
                                i * ATL_OFI_PMI_PROC_MULTIPLIER,
                                all_hostnames + i * ATL_OFI_MAX_HOSTNAME_LEN,
                                ATL_OFI_MAX_HOSTNAME_LEN);
        if (ret) {
            LOG_ERROR("pmrt_kvs_get: ret: ", ret);
            goto fn_err;
        }
    }

    for (i = 0; i < coord->global_count; i++) {
        if (!strncmp(my_hostname,
                     all_hostnames + i * ATL_OFI_MAX_HOSTNAME_LEN,
                     my_hostname_len + 1 /* including "-" at the end */)) {
            local_count++;
            int peer_global_proc_idx;
            sscanf(all_hostnames + i * ATL_OFI_MAX_HOSTNAME_LEN + my_hostname_len + 1,
                   "%d",
                   &peer_global_proc_idx);
            if (my_global_proc_idx > peer_global_proc_idx)
                local_idx++;
        }
    }

    coord->local_idx = local_idx;
    coord->local_count = local_count;

fn_exit:
    free(all_hostnames);
    return ret;

fn_err:
    ret = ATL_STATUS_FAILURE;
    goto fn_exit;
}

static atl_status_t atl_ofi_prov_update_addr_table(atl_ofi_ctx_t* ofi_ctx,
                                                   size_t prov_idx,
                                                   ipmi* pmi) {
    CCL_THROW_IF_NOT(ofi_ctx, "ofi_ctx is null");

    atl_ctx_t* ctx = &(ofi_ctx->ctx);
    atl_ofi_prov_t* prov = &(ofi_ctx->provs[prov_idx]);

    atl_status_t ret = ATL_STATUS_SUCCESS;
    int i;
    size_t j;
    int insert_count;

    size_t addr_idx = 0;
    char* ep_names_table;
    size_t ep_names_table_len;

    size_t named_ep_count = (prov->sep ? 1 : ctx->ep_count);

    int local_count = ctx->coord.local_count;
    int node_idx = ctx->coord.global_idx / local_count;
    int shm_start_idx = node_idx * local_count;
    int shm_end_idx = (node_idx + 1) * local_count;

    LOG_DEBUG("shm_start_idx ", shm_start_idx, ", shm_end_idx ", shm_end_idx);

    int proc_count = prov->is_shm ? ctx->coord.local_count : ctx->coord.global_count;

    if (proc_count == 0)
        return ATL_STATUS_SUCCESS;

    LOG_DEBUG("name ",
              prov->info->fabric_attr->prov_name,
              ", is_shm ",
              prov->is_shm,
              ", addr_len ",
              prov->addr_len,
              ", local_count ",
              ctx->coord.local_count,
              ", global_count ",
              ctx->coord.global_count,
              ", proc_count ",
              proc_count);

    /* allocate OFI EP names table that will contain all published names */
    ep_names_table_len = prov->addr_len * named_ep_count * proc_count;

    if (ep_names_table_len == 0) {
        LOG_ERROR("ep_names_table_len == 0, addr_len ",
                  prov->addr_len,
                  ", named_ep_count ",
                  named_ep_count,
                  ", proc_count ",
                  proc_count);
        return ATL_STATUS_FAILURE;
    }

    ep_names_table = (char*)calloc(1, ep_names_table_len);
    if (!ep_names_table) {
        LOG_ERROR("can't allocate epnames_table");
        return ATL_STATUS_FAILURE;
    }

    pmi->pmrt_barrier();

    /* retrieve all OFI EP names in order */
    for (i = 0; i < ctx->coord.global_count; i++) {
        if (prov->is_shm) {
            if (!(i >= shm_start_idx && i < shm_end_idx)) {
                continue;
            }
        }

        for (j = 0; j < named_ep_count; j++) {
            ret = pmi->pmrt_kvs_get(
                (char*)ATL_OFI_FI_ADDR_PM_KEY,
                i * ATL_OFI_PMI_PROC_MULTIPLIER + prov_idx * ATL_OFI_PMI_PROV_MULTIPLIER + j,
                ep_names_table + addr_idx * prov->addr_len,
                prov->addr_len);

            if (ret) {
                LOG_ERROR("kvs_get error: ret ",
                          ret,
                          ", proc_idx ",
                          i,
                          ", ep_idx ",
                          j,
                          ", addr_idx ",
                          addr_idx);
                goto err_ep_names;
            }

            addr_idx++;
        }
    }

    LOG_DEBUG(
        "kvs_get: ep_count ", named_ep_count, ", proc_count ", proc_count, ", got ", addr_idx);

    if (addr_idx != named_ep_count * proc_count) {
        LOG_ERROR("unexpected kvs_get results: expected ",
                  named_ep_count * proc_count,
                  ", got ",
                  addr_idx);

        ret = ATL_STATUS_FAILURE;
        goto err_addr_table;
    }

    if (prov->addr_table != NULL)
        free(prov->addr_table);

    prov->addr_table = (fi_addr_t*)calloc(1, ctx->ep_count * proc_count * sizeof(fi_addr_t));

    if (!prov->addr_table)
        goto err_ep_names;

    /* insert all the EP names into the AV */
    insert_count = fi_av_insert(
        prov->av, ep_names_table, named_ep_count * proc_count, prov->addr_table, 0, NULL);

    LOG_DEBUG("av_insert: ep_count ",
              named_ep_count,
              ", proc_count ",
              proc_count,
              ", inserted ",
              insert_count);

    if (insert_count != (int)(named_ep_count * proc_count)) {
        LOG_ERROR("unexpected av_insert results: expected ",
                  named_ep_count * proc_count,
                  " got ",
                  insert_count);
        ret = ATL_STATUS_FAILURE;
        goto err_addr_table;
    }
    else {
        ret = ATL_STATUS_SUCCESS;
    }

    if (prov->sep) {
        if (named_ep_count != 1) {
            LOG_ERROR("unexpected named_ep_count ", named_ep_count);
            goto err_addr_table;
        }

        fi_addr_t* table;
        table = (fi_addr_t*)calloc(1, proc_count * sizeof(fi_addr_t));
        if (table == NULL) {
            LOG_ERROR("memory allocaion failed");
            ret = ATL_STATUS_FAILURE;
            goto err_addr_table;
        }
        memcpy(table, prov->addr_table, proc_count * sizeof(fi_addr_t));

        for (i = 0; i < proc_count; i++) {
            for (j = 0; j < ctx->ep_count; j++) {
                prov->addr_table[i * ctx->ep_count + j] =
                    fi_rx_addr(table[i], j, prov->rx_ctx_bits);
            }
        }
        free(table);
    }

    /* normal end of execution */
    free(ep_names_table);
    return ret;

    /* abnormal end of execution */
err_addr_table:
    free(prov->addr_table);

err_ep_names:
    free(ep_names_table);
    return ret;
}

static atl_status_t atl_ofi_prov_ep_get_name(atl_ofi_prov_t* prov, size_t ep_idx) {
    int ret;

    atl_ofi_prov_ep_t* ep = &(prov->eps[ep_idx]);
    struct fid_ep* fi_ep = (prov->sep) ? prov->sep : ep->tx;

    ep->name.addr = NULL;
    ep->name.len = 0;

    ret = fi_getname(&fi_ep->fid, ep->name.addr, &(ep->name.len));
    if ((ret != -FI_ETOOSMALL) || ep->name.len <= 0)
        ep->name.len = FI_NAME_MAX;

    if (ep->name.addr)
        free(ep->name.addr);

    ep->name.addr = calloc(1, ep->name.len);

    if (!(ep->name.addr)) {
        LOG_ERROR("can't allocate addr");
        ret = ATL_STATUS_FAILURE;
        goto err_addr;
    }

    ret = fi_getname(&fi_ep->fid, ep->name.addr, &(ep->name.len));
    if (ret) {
        LOG_ERROR("fi_getname error");
        goto err_getname;
    }

    prov->addr_len = MAX(prov->addr_len, ep->name.len);

    return ATL_STATUS_SUCCESS;

err_getname:
    free(ep->name.addr);
    ep->name.addr = NULL;
    ep->name.len = 0;

err_addr:
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_prov_eps_connect(atl_ofi_ctx_t* ofi_ctx, size_t prov_idx, ipmi* pmi) {
    int ret;
    size_t ep_idx;

    atl_ctx_t* ctx = &(ofi_ctx->ctx);
    atl_ofi_prov_t* prov = &(ofi_ctx->provs[prov_idx]);
    size_t named_ep_count = (prov->sep ? 1 : ctx->ep_count);
    atl_proc_coord_t* coord = &(ctx->coord);

    prov->addr_len = 0;
    prov->first_proc_idx =
        (prov->is_shm) ? ((coord->global_idx / coord->local_count) * coord->local_count) : 0;

    for (ep_idx = 0; ep_idx < ctx->ep_count; ep_idx++) {
        ret = atl_ofi_prov_ep_get_name(prov, ep_idx);
        if (ret) {
            LOG_ERROR("atl_ofi_prov_ep_get_name error");
            return ATL_STATUS_FAILURE;
        }
    }

    for (ep_idx = 0; ep_idx < named_ep_count; ep_idx++) {
        atl_ofi_prov_ep_t* ep = &(prov->eps[ep_idx]);
        ret = pmi->pmrt_kvs_put((char*)ATL_OFI_FI_ADDR_PM_KEY,
                                coord->global_idx * ATL_OFI_PMI_PROC_MULTIPLIER +
                                    prov_idx * ATL_OFI_PMI_PROV_MULTIPLIER + ep_idx,
                                ep->name.addr,
                                ep->name.len);
        if (ret) {
            LOG_ERROR("pmrt_kvs_put: ret: ", ret);
            return ATL_STATUS_FAILURE;
        }
    }

    ret = atl_ofi_prov_update_addr_table(ofi_ctx, prov_idx, pmi);

    return RET2ATL(ret);
}

static void atl_ofi_prov_ep_destroy(atl_ofi_prov_t* prov, atl_ofi_prov_ep_t* ep) {
    if (ep->rx)
        fi_close(&ep->rx->fid);

    if (prov->sep && ep->tx)
        fi_close(&ep->tx->fid);

    if (ep->cq)
        fi_close(&ep->cq->fid);

    if (ep->name.addr)
        free(ep->name.addr);

    ep->rx = ep->tx = NULL;
    ep->cq = NULL;
    ep->name.addr = NULL;
    ep->name.len = 0;
}

static void atl_ofi_prov_destroy(atl_ctx_t* ctx, atl_ofi_prov_t* prov) {
    size_t i;

    for (i = 0; i < ctx->ep_count; i++) {
        atl_ofi_prov_ep_destroy(prov, &(prov->eps[i]));
    }

    free(prov->eps);
    free(prov->addr_table);

    if (prov->sep)
        fi_close(&prov->sep->fid);

    if (prov->av)
        fi_close(&prov->av->fid);

    if (prov->domain)
        fi_close(&prov->domain->fid);

    if (prov->fabric)
        fi_close(&prov->fabric->fid);

    if (prov->info) {
        fi_freeinfo(prov->info);
    }
}

static atl_status_t atl_ofi_prov_ep_handle_cq_err(atl_ofi_prov_ep_t* ep) {
    struct fi_cq_err_entry err_entry;
    atl_ofi_req_t* ofi_req;

    int ret = fi_cq_readerr(ep->cq, &err_entry, 0);
    if (ret != 1) {
        CCL_THROW("unable to read error from cq");
        return ATL_STATUS_FAILURE;
    }
    else {
        ofi_req = container_of(err_entry.op_context, atl_ofi_req_t, fi_ctx);

        if (err_entry.err == FI_ECANCELED) {
            return ATL_STATUS_SUCCESS;
        }

        if (err_entry.err == FI_ENOMSG && ofi_req->comp_state == ATL_OFI_COMP_PEEK_STARTED) {
            ofi_req->comp_state = ATL_OFI_COMP_PEEK_NOT_FOUND;
        }
        else {
            LOG_ERROR("fi_cq_readerr: err: ",
                      err_entry.err,
                      ", prov_err: ",
                      fi_cq_strerror(ep->cq, err_entry.prov_errno, err_entry.err_data, NULL, 0),
                      "(",
                      err_entry.prov_errno,
                      ")");
            return ATL_STATUS_FAILURE;
        }
        return ATL_STATUS_SUCCESS;
    }
}

static inline void atl_ofi_process_comps(struct fi_cq_tagged_entry* entries, ssize_t ret) {
    ssize_t idx;
    atl_ofi_req_t* comp_ofi_req;
    for (idx = 0; idx < ret; idx++) {
        comp_ofi_req = container_of(entries[idx].op_context, atl_ofi_req_t, fi_ctx);
        switch (comp_ofi_req->comp_state) {
            case ATL_OFI_COMP_POSTED: comp_ofi_req->comp_state = ATL_OFI_COMP_COMPLETED; break;
            case ATL_OFI_COMP_COMPLETED: break;
            case ATL_OFI_COMP_PEEK_STARTED:
                comp_ofi_req->comp_state = ATL_OFI_COMP_PEEK_FOUND;
                break;
            default: CCL_THROW("unexpected completion state ", comp_ofi_req->comp_state); break;
        }

        if (entries[idx].flags & FI_RECV) {
            comp_ofi_req->recv_len = entries[idx].len;
        }
    }
}

static int atl_ofi_wait_cancel_cq(struct fid_cq* cq) {
    struct fi_cq_err_entry err_entry;
    int ret, i;
    struct fi_cq_tagged_entry entries[ATL_OFI_CQ_BUNCH_SIZE];

    double time = 0;
    clock_t start, end;

    while (time < ATL_OFI_WAIT_SEC) {
        for (i = 0; i < ATL_OFI_CQ_READ_ITERS; i++) {
            start = clock();
            ret = fi_cq_read(cq, entries, ATL_OFI_CQ_BUNCH_SIZE);

            if (ret < 0 && ret != -FI_EAGAIN) {
                ret = fi_cq_readerr(cq, &err_entry, 0);

                if (err_entry.err != FI_ECANCELED) {
                    LOG_ERROR("fi_cq_readerr: err: ",
                              err_entry.err,
                              ", prov_err: ",
                              fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0),
                              "(",
                              err_entry.prov_errno,
                              ")");
                    return ATL_STATUS_FAILURE;
                }
                return ATL_STATUS_SUCCESS;
            }
        }
        end = clock();
        time += (double)(end - start) / CLOCKS_PER_SEC;
    }

    LOG_ERROR("too long for cancel");

    return ATL_STATUS_FAILURE;
}

static atl_status_t atl_ofi_prov_ep_init(atl_ofi_prov_t* prov, size_t ep_idx) {
    ssize_t ret = 0;

    struct fi_cq_attr cq_attr;
    struct fi_tx_attr tx_attr;
    struct fi_rx_attr rx_attr;

    atl_ofi_prov_ep_t* ep = &(prov->eps[ep_idx]);

    memset(&cq_attr, 0, sizeof(cq_attr));
    cq_attr.format = FI_CQ_FORMAT_TAGGED;

    ATL_OFI_CALL(fi_cq_open(prov->domain, &cq_attr, &ep->cq, NULL), ret, return ATL_STATUS_FAILURE);

    if (prov->sep) {
        rx_attr = *prov->info->rx_attr;
        rx_attr.caps |= FI_TAGGED;

        ATL_OFI_CALL(fi_rx_context(prov->sep, ep_idx, &rx_attr, &ep->rx, NULL), ret, goto err);

        ATL_OFI_CALL(fi_ep_bind(ep->rx, &ep->cq->fid, FI_RECV), ret, goto err);

        tx_attr = *prov->info->tx_attr;
        tx_attr.caps |= FI_TAGGED;

        ATL_OFI_CALL(fi_tx_context(prov->sep, ep_idx, &tx_attr, &ep->tx, NULL), ret, goto err);

        ATL_OFI_CALL(fi_ep_bind(ep->tx, &ep->cq->fid, FI_SEND), ret, goto err);

        fi_enable(ep->rx);
        fi_enable(ep->tx);
    }
    else {
        struct fid_ep* endpoint;

        ATL_OFI_CALL(fi_endpoint(prov->domain, prov->info, &endpoint, NULL), ret, goto err);

        ep->tx = ep->rx = endpoint;

        ATL_OFI_CALL(fi_ep_bind(endpoint, &ep->cq->fid, FI_SEND | FI_RECV), ret, goto err);

        ATL_OFI_CALL(fi_ep_bind(endpoint, &prov->av->fid, 0), ret, goto err);

        fi_enable(endpoint);
    }

    return ATL_STATUS_SUCCESS;

err:
    atl_ofi_prov_ep_destroy(prov, ep);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_try_to_drain_cq_err(struct fid_cq* cq) {
    struct fi_cq_err_entry err_entry;
    int ret = fi_cq_readerr(cq, &err_entry, 0);
    if (ret != 1) {
        LOG_DEBUG("unable to fi_cq_readerr");
        return ATL_STATUS_FAILURE;
    }
    else {
        if (err_entry.err != FI_ENOMSG && err_entry.err != FI_ECANCELED &&
            err_entry.err != FI_ETRUNC) {
            LOG_ERROR("fi_cq_readerr: err: ",
                      err_entry.err,
                      ", prov_err: ",
                      fi_cq_strerror(cq, err_entry.prov_errno, err_entry.err_data, NULL, 0),
                      "(",
                      err_entry.prov_errno,
                      ")");
            return ATL_STATUS_FAILURE;
        }
        return ATL_STATUS_SUCCESS;
    }
}

static int atl_ofi_try_to_drain_cq(struct fid_cq* cq) {
    int ret = -FI_EAGAIN, i;
    double time = 0;
    clock_t start, end;
    struct fi_cq_tagged_entry entries[ATL_OFI_CQ_BUNCH_SIZE];

    while (time < ATL_OFI_WAIT_SEC) {
        start = clock();
        for (i = 0; i < ATL_OFI_CQ_READ_ITERS; i++) {
            ret = fi_cq_read(cq, entries, ATL_OFI_CQ_BUNCH_SIZE);

            if (ret < 0 && ret != -FI_EAGAIN) {
                atl_ofi_try_to_drain_cq_err(cq);
                return ret;
            }

            if (ret > 0)
                return ret;
        }
        end = clock();
        time += (double)(end - start) / CLOCKS_PER_SEC;
    }

    return ret;
}

static void atl_ofi_reset(atl_ctx_t* ctx) {
    atl_ofi_ctx_t* ofi_ctx = container_of(ctx, atl_ofi_ctx_t, ctx);

    int again = 1;
    size_t prov_idx, ep_idx;
    int recv_buf_len = sizeof(char);
    char* recv_buf;
    struct fi_context fi_ctx;
    recv_buf = (char*)malloc(recv_buf_len);
    for (prov_idx = 0; prov_idx < ofi_ctx->prov_count; prov_idx++) {
        atl_ofi_prov_t* prov = &(ofi_ctx->provs[prov_idx]);

        for (ep_idx = 0; ep_idx < ctx->ep_count; ep_idx++) {
            atl_ofi_prov_ep_t* ep = &(prov->eps[ep_idx]);

            /* complete active sends and receives */
            while (atl_ofi_try_to_drain_cq(ep->cq) != -FI_EAGAIN) {
            }

            /* try to complete active incoming sends */
            while (again) {
                again = 0;
                /* post recv to complete incoming send */
                while (fi_trecv(ep->rx,
                                recv_buf,
                                recv_buf_len,
                                NULL,
                                FI_ADDR_UNSPEC,
                                0,
                                UINTMAX_MAX,
                                &fi_ctx) == -FI_EAGAIN) {
                }

                /* wait until recv will be completed or finished by timeout */
                while (atl_ofi_try_to_drain_cq(ep->cq) != -FI_EAGAIN) {
                    /* something is completed -> send queue not empty */
                    again = 1;
                }
            }

            /* nothing to recv -> cancel last recv */
            fi_cancel(&ep->rx->fid, &fi_ctx);

            atl_ofi_wait_cancel_cq(ep->cq);
        }
    }

    free(recv_buf);
}

static atl_status_t atl_ofi_adjust_env(const atl_attr_t& attr) {
    char* prov_env = getenv("FI_PROVIDER");

    if (prov_env && strlen(prov_env)) {
        CCL_THROW_IF_NOT(strlen(prov_env) < sizeof(global_data.prov_env_copy),
                         "too long FI_PROVIDER value, max expected length ",
                         sizeof(global_data.prov_env_copy));
        memcpy(global_data.prov_env_copy, prov_env, strlen(prov_env));
    }

    if (attr.enable_shm) {
        /* add shm provider in the list of allowed providers */
        if (prov_env && !strstr(prov_env, ATL_OFI_SHM_PROV_NAME)) {
            /* whether single provider will be in the final env variable */
            int single_prov = (strlen(prov_env) == 0) ? 1 : 0;

            size_t prov_env_new_size = strlen(prov_env) + strlen(ATL_OFI_SHM_PROV_NAME) +
                                       (single_prov ? 0 : 1) + /* for delimeter */
                                       1; /* for terminating null symbol */

            char* prov_env_new = (char*)calloc(prov_env_new_size, sizeof(char));
            if (prov_env_new == NULL) {
                LOG_ERROR("memory allocaion failed");
                return ATL_STATUS_FAILURE;
            }

            if (single_prov)
                snprintf(prov_env_new, prov_env_new_size, "%s", ATL_OFI_SHM_PROV_NAME);
            else {
                snprintf(prov_env_new, prov_env_new_size, "%s,%s", prov_env, ATL_OFI_SHM_PROV_NAME);
            }

            LOG_INFO("ATL/SHM is requested, modify FI_PROVIDER: old value: ",
                     prov_env,
                     ", new value: ",
                     prov_env_new);

            setenv("FI_PROVIDER", prov_env_new, 1);

            free(prov_env_new);
        }
    }

    return ATL_STATUS_SUCCESS;
}

static atl_status_t atl_ofi_set_env(const atl_attr_t& attr) {
    if (global_data.is_env_inited) {
        return ATL_STATUS_SUCCESS;
    }

    setenv("FI_PSM2_DELAY", "0", 0);
    setenv("FI_PSM2_TIMEOUT", "0", 0);
    setenv("FI_PSM2_LOCK_LEVEL", "1", 0);
    setenv("FI_PSM2_NAME_SERVER", "0", 0);
    setenv("HFI_NO_CPUAFFINITY", "1", 0);
    setenv("PSM2_MULTI_EP", "1", 0);

    setenv("FI_OFI_RXM_USE_HASH", "0", 0);
    setenv("FI_OFI_RXM_RX_SIZE", "8192", 0);
    setenv("FI_OFI_RXM_TX_SIZE", "8192", 0);
    setenv("FI_OFI_RXM_MSG_RX_SIZE", "128", 0);
    setenv("FI_OFI_RXM_MSG_TX_SIZE", "128", 0);

    setenv("FI_SHM_TX_SIZE", "8192", 0);
    setenv("FI_SHM_RX_SIZE", "8192", 0);

    atl_ofi_adjust_env(attr);

    global_data.is_env_inited = 1;

    return ATL_STATUS_SUCCESS;
}

static atl_status_t atl_ofi_finalize(atl_ctx_t* ctx) {
    int ret = 0;
    size_t idx;

    atl_ofi_ctx_t* ofi_ctx = container_of(ctx, atl_ofi_ctx_t, ctx);

    global_data.ctx_count--;
    if (ctx->coord.global_idx == 0) {
        LOG_INFO("finalize atl-ofi ctx, remaining ctx_count ", global_data.ctx_count);
    }

    for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
        atl_ofi_prov_t* prov = &ofi_ctx->provs[idx];
        atl_ofi_prov_destroy(ctx, prov);
    }

    for (idx = 0; idx < ctx->ep_count; idx++) {
        atl_ofi_ep_t* ofi_ep = container_of(ctx->eps[idx], atl_ofi_ep_t, ep);
        free(ofi_ep);
    }

    if (global_data.ctx_count == 0) {
        if (ctx->coord.global_idx == 0) {
            LOG_INFO("finalized last atl-ofi ctx");
        }
    }

    free(ctx->eps);
    free(ofi_ctx);

    return RET2ATL(ret);
}

static atl_status_t atl_ofi_mr_reg(atl_ctx_t* ctx, const void* buf, size_t len, atl_mr_t** mr) {
    int ret;
    atl_ofi_ctx_t* ofi_ctx;
    ofi_ctx = container_of(ctx, atl_ofi_ctx_t, ctx);
    atl_ofi_prov_t* prov = &(ofi_ctx->provs[0]);

    atl_ofi_mr_t* ofi_mr;
    ofi_mr = (atl_ofi_mr_t*)calloc(1, sizeof(atl_ofi_mr_t));
    if (!ofi_mr)
        return ATL_STATUS_FAILURE;

    ret = fi_mr_reg(prov->domain,
                    buf,
                    len,
                    FI_SEND | FI_RECV | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE,
                    0,
                    0,
                    0,
                    &ofi_mr->fi_mr,
                    NULL);
    if (ret)
        goto mr_reg_err;

    ofi_mr->mr.buf = (void*)buf;
    ofi_mr->mr.len = len;
    ofi_mr->mr.remote_key = (uintptr_t)fi_mr_key(ofi_mr->fi_mr);
    ofi_mr->mr.local_key = (uintptr_t)fi_mr_desc(ofi_mr->fi_mr);

    *mr = &ofi_mr->mr;
    return ATL_STATUS_SUCCESS;

mr_reg_err:
    free(ofi_mr);
    return ATL_STATUS_FAILURE;
}

static atl_status_t atl_ofi_mr_dereg(atl_ctx_t* ctx, atl_mr_t* mr) {
    atl_ofi_mr_t* ofi_mr;
    ofi_mr = container_of(mr, atl_ofi_mr_t, mr);
    int ret = fi_close(&ofi_mr->fi_mr->fid);
    free(ofi_mr);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_ep_wait(atl_ep_t* ep, atl_req_t* req);

static atl_status_t atl_ofi_ep_send(atl_ep_t* ep,
                                    const void* buf,
                                    size_t len,
                                    int dst_proc_idx,
                                    uint64_t tag,
                                    atl_req_t* req) {
    ssize_t ret;

    atl_ofi_prov_t* prov;
    atl_ofi_prov_ep_t* prov_ep;
    atl_ofi_req_t* ofi_req;

    prov = atl_ofi_get_prov(ep, dst_proc_idx, len);
    prov_ep = &(prov->eps[ep->idx]);
    ofi_req = ((atl_ofi_req_t*)req->internal);

    req->tag = tag;
    req->remote_proc_idx = dst_proc_idx;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ofi_req->prov_ep = prov_ep;
    ofi_req->fi_ep = prov_ep->tx;

    ATL_OFI_RETRY(fi_tsend(prov_ep->tx,
                           buf,
                           len,
                           NULL,
                           atl_ofi_get_addr(ep->ctx, prov, dst_proc_idx, ep->idx),
                           tag,
                           &ofi_req->fi_ctx),
                  ep,
                  ret);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_ep_recv(atl_ep_t* ep,
                                    void* buf,
                                    size_t len,
                                    int src_proc_idx,
                                    uint64_t tag,
                                    atl_req_t* req) {
    ssize_t ret;

    atl_ofi_prov_t* prov;
    atl_ofi_prov_ep_t* prov_ep;
    atl_ofi_req_t* ofi_req;

    prov = atl_ofi_get_prov(ep, src_proc_idx, len);
    prov_ep = &(prov->eps[ep->idx]);
    ofi_req = ((atl_ofi_req_t*)req->internal);

    req->tag = tag;
    req->remote_proc_idx = src_proc_idx;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ofi_req->prov_ep = prov_ep;
    ofi_req->fi_ep = prov_ep->rx;

    ATL_OFI_RETRY(fi_trecv(prov_ep->rx,
                           buf,
                           len,
                           NULL,
                           atl_ofi_get_addr(ep->ctx, prov, src_proc_idx, ep->idx),
                           tag,
                           0,
                           &ofi_req->fi_ctx),
                  ep,
                  ret);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_ep_probe(atl_ep_t* ep,
                                     int src_proc_idx,
                                     uint64_t tag,
                                     int* found,
                                     size_t* recv_len) {
    atl_status_t ret;
    atl_ofi_req_t reqs[ATL_OFI_MAX_PROV_COUNT];
    struct fi_msg_tagged msgs[ATL_OFI_MAX_PROV_COUNT];
    int flag, len;
    ssize_t ofi_ret;
    size_t idx;
    int do_poll;

    atl_ofi_ctx_t* ofi_ctx;

    ret = ATL_STATUS_SUCCESS;
    flag = 0;
    len = 0;
    ofi_ret = FI_SUCCESS;
    do_poll = 1;

    ofi_ctx = container_of(ep->ctx, atl_ofi_ctx_t, ctx);

    for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
        atl_ofi_prov_t* prov;
        atl_ofi_prov_ep_t* prov_ep;
        atl_ofi_req_t* req;
        struct fi_msg_tagged* msg;

        prov = &(ofi_ctx->provs[idx]);
        prov_ep = &(prov->eps[ep->idx]);
        req = &(reqs[idx]);
        msg = &(msgs[idx]);

        if (prov->is_shm &&
            ((src_proc_idx < prov->first_proc_idx) ||
             (src_proc_idx >= (prov->first_proc_idx + ep->ctx->coord.local_count)))) {
            req->prov_ep = NULL;
            continue;
        }

        req->comp_state = ATL_OFI_COMP_PEEK_STARTED;
        req->prov_ep = prov_ep;
        req->fi_ep = prov_ep->rx;

        msg->msg_iov = NULL;
        msg->desc = NULL;
        msg->iov_count = 0;
        msg->addr = atl_ofi_get_addr(ep->ctx, prov, src_proc_idx, ep->idx);
        msg->tag = tag;
        msg->ignore = 0;
        msg->context = &(req->fi_ctx);
        msg->data = 0;

        ATL_OFI_RETRY(fi_trecvmsg(prov_ep->rx, msg, FI_PEEK | FI_COMPLETION), ep, ofi_ret);
    }

    do {
        ret = atl_ofi_ep_poll(ep);
        if (ret != ATL_STATUS_SUCCESS)
            return ret;

        for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
            atl_ofi_req_t* req;
            req = &(reqs[idx]);

            if (!req->prov_ep)
                continue;

            if (req->comp_state != ATL_OFI_COMP_PEEK_STARTED) {
                do_poll = 0;

                if (req->comp_state == ATL_OFI_COMP_PEEK_FOUND) {
                    flag = 1;
                    len = req->recv_len;
                    req->prov_ep = NULL;
                }
                else if (req->comp_state == ATL_OFI_COMP_PEEK_NOT_FOUND) {
                    req->prov_ep = NULL;
                }
                else {
                    CCL_THROW("unexpected completion state ", req->comp_state);
                }

                break;
            }
        }
    } while (do_poll);

    for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
        atl_ofi_req_t* req;
        req = &(reqs[idx]);

        if (!req->prov_ep)
            continue;

        if (fi_cancel(&req->fi_ep->fid, &req->fi_ctx) == 0) {
            atl_ofi_wait_cancel_cq(req->prov_ep->cq);
        }
    }

    if (found)
        *found = flag;
    if (recv_len)
        *recv_len = len;

    return RET2ATL(ofi_ret);
}

static atl_status_t atl_ofi_ep_allgatherv(atl_ep_t* ep,
                                          const void* send_buf,
                                          size_t send_len,
                                          void* recv_buf,
                                          const int* recv_lens,
                                          const int* offsets,
                                          atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_allreduce(atl_ep_t* ep,
                                         const void* send_buf,
                                         void* recv_buf,
                                         size_t count,
                                         atl_datatype_t dtype,
                                         atl_reduction_t op,
                                         atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_alltoall(atl_ep_t* ep,
                                        const void* send_buf,
                                        void* recv_buf,
                                        size_t len,
                                        atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_alltoallv(atl_ep_t* ep,
                                         const void* send_buf,
                                         const int* send_lens,
                                         const int* send_offsets,
                                         void* recv_buf,
                                         const int* recv_lens,
                                         const int* recv_offsets,
                                         atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_barrier(atl_ep_t* ep, atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_bcast(atl_ep_t* ep,
                                     void* buf,
                                     size_t len,
                                     int root,
                                     atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_reduce(atl_ep_t* ep,
                                      const void* send_buf,
                                      void* recv_buf,
                                      size_t count,
                                      int root,
                                      atl_datatype_t dtype,
                                      atl_reduction_t op,
                                      atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_reduce_scatter(atl_ep_t* ep,
                                              const void* send_buf,
                                              void* recv_buf,
                                              size_t recv_count,
                                              atl_datatype_t dtype,
                                              atl_reduction_t op,
                                              atl_req_t* req) {
    return ATL_STATUS_UNSUPPORTED;
}

static atl_status_t atl_ofi_ep_read(atl_ep_t* ep,
                                    void* buf,
                                    size_t len,
                                    atl_mr_t* mr,
                                    uint64_t addr,
                                    uintptr_t remote_key,
                                    int dst_proc_idx,
                                    atl_req_t* req) {
    ssize_t ret;

    atl_ofi_prov_t* prov;
    atl_ofi_prov_ep_t* prov_ep;
    atl_ofi_req_t* ofi_req;

    prov = atl_ofi_get_prov(ep, dst_proc_idx, len);
    prov_ep = &(prov->eps[ep->idx]);
    ofi_req = ((atl_ofi_req_t*)req->internal);

    req->tag = 0;
    req->remote_proc_idx = dst_proc_idx;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ofi_req->prov_ep = prov_ep;
    ofi_req->fi_ep = prov_ep->tx;

    ATL_OFI_RETRY(fi_read(prov_ep->tx,
                          buf,
                          len,
                          (void*)mr->local_key,
                          atl_ofi_get_addr(ep->ctx, prov, dst_proc_idx, ep->idx),
                          addr,
                          remote_key,
                          &ofi_req->fi_ctx),
                  ep,
                  ret);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_ep_write(atl_ep_t* ep,
                                     const void* buf,
                                     size_t len,
                                     atl_mr_t* mr,
                                     uint64_t addr,
                                     uintptr_t remote_key,
                                     int dst_proc_idx,
                                     atl_req_t* req) {
    ssize_t ret;

    atl_ofi_prov_t* prov;
    atl_ofi_prov_ep_t* prov_ep;
    atl_ofi_req_t* ofi_req;

    prov = atl_ofi_get_prov(ep, dst_proc_idx, len);
    prov_ep = &(prov->eps[ep->idx]);
    ofi_req = ((atl_ofi_req_t*)req->internal);

    req->tag = 0;
    req->remote_proc_idx = dst_proc_idx;
    ofi_req->comp_state = ATL_OFI_COMP_POSTED;

    ofi_req->prov_ep = prov_ep;
    ofi_req->fi_ep = prov_ep->tx;

    ATL_OFI_RETRY(fi_write(prov_ep->tx,
                           buf,
                           len,
                           (void*)mr->local_key,
                           atl_ofi_get_addr(ep->ctx, prov, dst_proc_idx, ep->idx),
                           addr,
                           remote_key,
                           &ofi_req->fi_ctx),
                  ep,
                  ret);
    return RET2ATL(ret);
}

static atl_status_t atl_ofi_ep_wait(atl_ep_t* ep, atl_req_t* req) {
    atl_status_t ret;
    atl_ofi_req_t* ofi_req;

    ret = ATL_STATUS_SUCCESS;
    ofi_req = ((atl_ofi_req_t*)req->internal);

    while ((ofi_req->comp_state != ATL_OFI_COMP_COMPLETED) &&
           ((ret = atl_ofi_ep_poll(ep)) == ATL_STATUS_SUCCESS))
        ;

    return ret;
}

static atl_status_t atl_ofi_ep_wait_all(atl_ep_t* ep, atl_req_t* reqs, size_t count) {
    size_t i;
    atl_status_t ret;

    for (i = 0; i < count; i++) {
        ret = atl_ofi_ep_wait(ep, &reqs[i]);
        if (ret != ATL_STATUS_SUCCESS)
            return ret;
    }

    return ATL_STATUS_SUCCESS;
}

static atl_status_t atl_ofi_ep_cancel(atl_ep_t* ep, atl_req_t* req) {
    int ret;
    atl_ofi_req_t* ofi_req;

    ret = ATL_STATUS_SUCCESS;
    ofi_req = ((atl_ofi_req_t*)req->internal);

    ret = fi_cancel(&ofi_req->fi_ep->fid, &ofi_req->fi_ctx);
    if (ret == 0) {
        return RET2ATL(atl_ofi_wait_cancel_cq(ofi_req->prov_ep->cq));
    }

    return ATL_STATUS_SUCCESS;
}

static inline atl_status_t atl_ofi_ep_progress(atl_ep_t* ep, atl_ofi_req_t* req /* unused */) {
    ssize_t ret;
    size_t idx;
    struct fi_cq_tagged_entry entries[ATL_OFI_CQ_BUNCH_SIZE];

    atl_ofi_ctx_t* ofi_ctx;
    size_t ep_idx;

    ofi_ctx = container_of(ep->ctx, atl_ofi_ctx_t, ctx);
    ep_idx = ep->idx;

    /* ensure progress for all providers */
    for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
        atl_ofi_prov_ep_t* prov_ep;
        prov_ep = &(ofi_ctx->provs[idx].eps[ep_idx]);
        do {
            ret = fi_cq_read(prov_ep->cq, entries, ATL_OFI_CQ_BUNCH_SIZE);
            if (ret > 0)
                atl_ofi_process_comps(entries, ret);
            else if (ret == -FI_EAGAIN)
                break;
            else
                return atl_ofi_prov_ep_handle_cq_err(prov_ep);
        } while (ret > 0);
    }

    return ATL_STATUS_SUCCESS;
}

static inline atl_status_t atl_ofi_ep_poll(atl_ep_t* ep) {
    atl_ofi_ctx_t* ofi_ctx = container_of(ep->ctx, atl_ofi_ctx_t, ctx);
    if (ofi_ctx->progress_mode == ATL_PROGRESS_POLL) {
        atl_ofi_ep_progress(ep, NULL /* ofi_req */);
    }
    return ATL_STATUS_SUCCESS;
}

static atl_status_t atl_ofi_ep_check(atl_ep_t* ep, int* is_completed, atl_req_t* req) {
    CCL_THROW_IF_NOT(is_completed);

    atl_status_t status;
    atl_ofi_req_t* ofi_req;
    atl_ofi_ctx_t* ofi_ctx = container_of(ep->ctx, atl_ofi_ctx_t, ctx);

    status = ATL_STATUS_SUCCESS;
    ofi_req = ((atl_ofi_req_t*)req->internal);

    *is_completed = (ofi_req->comp_state == ATL_OFI_COMP_COMPLETED);
    if (*is_completed) {
        return ATL_STATUS_SUCCESS;
    }

    if (ofi_ctx->progress_mode == ATL_PROGRESS_CHECK) {
        status = atl_ofi_ep_progress(ep, ofi_req);
        *is_completed = (ofi_req->comp_state == ATL_OFI_COMP_COMPLETED);
    }

    return status;
}

static atl_ops_t atl_ofi_ops = {
    .finalize = atl_ofi_finalize,
};

static atl_mr_ops_t atl_ofi_mr_ops = {
    .mr_reg = atl_ofi_mr_reg,
    .mr_dereg = atl_ofi_mr_dereg,
};

static atl_p2p_ops_t atl_ofi_ep_p2p_ops = {
    .send = atl_ofi_ep_send,
    .recv = atl_ofi_ep_recv,
    .probe = atl_ofi_ep_probe,
};

static atl_coll_ops_t atl_ofi_ep_coll_ops = {
    .allgatherv = atl_ofi_ep_allgatherv,
    .allreduce = atl_ofi_ep_allreduce,
    .alltoall = atl_ofi_ep_alltoall,
    .alltoallv = atl_ofi_ep_alltoallv,
    .barrier = atl_ofi_ep_barrier,
    .bcast = atl_ofi_ep_bcast,
    .reduce = atl_ofi_ep_reduce,
    .reduce_scatter = atl_ofi_ep_reduce_scatter,
};

static atl_rma_ops_t atl_ofi_ep_rma_ops = {
    .read = atl_ofi_ep_read,
    .write = atl_ofi_ep_write,
};

static atl_comp_ops_t atl_ofi_ep_comp_ops = { .wait = atl_ofi_ep_wait,
                                              .wait_all = atl_ofi_ep_wait_all,
                                              .cancel = atl_ofi_ep_cancel,
                                              .poll = atl_ofi_ep_poll,
                                              .check = atl_ofi_ep_check };

static atl_status_t atl_ofi_init(int* argc,
                                 char*** argv,
                                 atl_attr_t* attr,
                                 atl_ctx_t** out_ctx,
                                 const char* main_addr,
                                 ipmi* pmi) {
    struct fi_info *providers, *base_hints, *prov_hints;
    struct fi_av_attr av_attr;
    int fi_version;
    ssize_t ret;
    size_t idx, ep_idx;

    providers = NULL;
    base_hints = NULL;
    prov_hints = NULL;

    memset(&av_attr, 0, sizeof(av_attr));

    ret = 0;
    idx = 0;
    ep_idx = 0;

    CCL_THROW_IF_NOT((sizeof(atl_ofi_req_t) <= sizeof(atl_req_t) - offsetof(atl_req_t, internal)),
                     "unexpected offset: atl_ofi_request size ",
                     sizeof(atl_ofi_req_t),
                     ", atl_request size ",
                     sizeof(atl_req_t),
                     ", expected offset ",
                     offsetof(atl_req_t, internal));

    if (global_data.ctx_count == 0) {
        ret = atl_ofi_set_env(*attr);
        if (ret != ATL_STATUS_SUCCESS) {
            LOG_ERROR("atl_ofi_set_env error");
            return ATL_STATUS_FAILURE;
        }
    }
    global_data.ctx_count++;

    atl_ofi_ctx_t* ofi_ctx;
    ofi_ctx = (atl_ofi_ctx_t*)calloc(1, sizeof(atl_ofi_ctx_t));
    if (!ofi_ctx)
        return ATL_STATUS_FAILURE;

    atl_ctx_t* ctx = &(ofi_ctx->ctx);

    ctx->ops = &atl_ofi_ops;
    ctx->mr_ops = &atl_ofi_mr_ops;
    ctx->ep_count = attr->ep_count;
    ctx->eps = (atl_ep**)calloc(1, sizeof(void*) * attr->ep_count);
    if (!ctx->eps)
        goto err;

    ctx->coord.global_count = pmi->get_size();
    ctx->coord.global_idx = pmi->get_rank();

    ret = atl_ofi_get_local_proc_coord(ofi_ctx, pmi);
    if (ret) {
        LOG_ERROR("atl_ofi_get_local_proc_coord error");
        goto err;
    }

    atl_proc_coord_t* coord;
    coord = &(ctx->coord);

    base_hints = fi_allocinfo();
    if (!base_hints) {
        LOG_ERROR("can't alloc base_hints");
        goto err;
    }

    base_hints->mode = FI_CONTEXT;
    base_hints->ep_attr->type = FI_EP_RDM;
    base_hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    base_hints->domain_attr->control_progress = FI_PROGRESS_MANUAL;
    base_hints->domain_attr->data_progress = FI_PROGRESS_MANUAL;
    base_hints->caps = FI_TAGGED;
    base_hints->caps |= FI_DIRECTED_RECV;

    fi_version = FI_VERSION(FI_MAJOR_VERSION, FI_MINOR_VERSION);

    if (coord->global_idx == 0)
        LOG_INFO("libfabric version: ", fi_tostr("1" /* ignored */, FI_TYPE_VERSION));

    char* prov_env;
    prov_env = getenv("FI_PROVIDER");
    if (prov_env && !strcmp(prov_env, ATL_OFI_SHM_PROV_NAME)) {
        if (coord->global_count != coord->local_count) {
            LOG_ERROR("shm provider is requested as primary provider but global_count (",
                      coord->global_count,
                      ") != local_count (",
                      coord->local_count,
                      ")");
            goto err;
        }

        if (!attr->enable_shm) {
            LOG_ERROR(
                "shm provider is requested through FI_PROVIDER but not requested from CCL level");
            goto err;
        }
    }
    atl_ofi_print_coord(coord);

    if (attr->enable_shm) {
        prov_hints = fi_dupinfo(base_hints);
        prov_hints->fabric_attr->prov_name = strdup(ATL_OFI_SHM_PROV_NAME);
        ret = fi_getinfo(fi_version, NULL, NULL, 0ULL, prov_hints, &providers);
        if (ret || !providers) {
            attr->enable_shm = 0;
            LOG_INFO("shm provider is requested but not available");
        }
        else {
            LOG_INFO("shm provider is requested and available");
        }

        fi_freeinfo(providers);
        providers = NULL;

        fi_freeinfo(prov_hints);
        prov_hints = NULL;

        if (attr->enable_shm) {
            /* TODO: tmp code to detect CMA, remove when OFI/shm will have runtime detection */
            int scope = 0, fret;
            FILE* file;
            file = fopen("/proc/sys/kernel/yama/ptrace_scope", "r");
            if (file) {
                fret = fscanf(file, "%d", &scope);
                if (fret != 1) {
                    LOG_ERROR("error getting value from ptrace_scope");
                    scope = 1;
                }
                fret = fclose(file);
                if (fret) {
                    LOG_ERROR("error closing ptrace_scope file");
                    scope = 1;
                }
            }

            if (!file && (errno != ENOENT)) {
                LOG_ERROR("can't open ptrace_scope file, disable shm provider");
                attr->enable_shm = 0;
            }
            else if (scope) {
                LOG_ERROR("ptrace_scope > 0, disable shm provider");
                attr->enable_shm = 0;
            }
        }
    }

    attr->tag_bits = 64;
    attr->max_tag = 0xFFFFFFFFFFFFFFFF;

    if (coord->global_count == coord->local_count) {
        ofi_ctx->prov_count = 1;
        ofi_ctx->provs[0].is_shm = (attr->enable_shm) ? 1 : 0;
    }
    else {
        if (attr->enable_shm) {
            ofi_ctx->prov_count = 2;
            ofi_ctx->shm_prov_idx = 0;
            ofi_ctx->nw_prov_idx = 1;
            ofi_ctx->provs[ofi_ctx->shm_prov_idx].is_shm = 1;
            ofi_ctx->provs[ofi_ctx->nw_prov_idx].is_shm = 0;
        }
        else {
            ofi_ctx->prov_count = 1;
            ofi_ctx->provs[0].is_shm = 0;
        }
    }

    if (attr->enable_rma && (ofi_ctx->prov_count > 1)) {
        LOG_INFO("RMA and multiple providers requested both, disable RMA");
        attr->enable_rma = 0;
    }

    if (coord->global_idx == 0)
        LOG_INFO("prov_count ", ofi_ctx->prov_count);

    for (idx = 0; idx < ofi_ctx->prov_count; idx++) {
        atl_ofi_prov_t* prov;
        prov = &ofi_ctx->provs[idx];

        prov_hints = fi_dupinfo(base_hints);

        char* prov_name = NULL;

        if (prov->is_shm)
            prov_name = strdup(ATL_OFI_SHM_PROV_NAME);
        else {
            if (strlen(global_data.prov_env_copy) && !strstr(global_data.prov_env_copy, ","))
                prov_name = strdup(global_data.prov_env_copy);
            else
                prov_name = NULL;
        }

        LOG_DEBUG("request provider: idx ",
                  idx,
                  ", name ",
                  (prov_name) ? prov_name : "<default>",
                  ", is_shm ",
                  prov->is_shm);

        prov_hints->fabric_attr->prov_name = prov_name;

        ret = fi_getinfo(fi_version, NULL, NULL, 0ULL, prov_hints, &providers);

        if (ret || !providers) {
            LOG_ERROR(
                "fi_getinfo erro: ret ", ret, ", providers ", (void*)providers, ", prov_idx ", idx);
            goto err;
        }

        if (providers->domain_attr->max_ep_tx_ctx > 1) {
            prov_hints->ep_attr->tx_ctx_cnt = attr->ep_count;
            prov_hints->ep_attr->rx_ctx_cnt = attr->ep_count;
        }
        else {
            prov_hints->ep_attr->tx_ctx_cnt = 1;
            prov_hints->ep_attr->rx_ctx_cnt = 1;
        }

        fi_freeinfo(providers);
        providers = NULL;

        if (attr->enable_rma) {
            LOG_INFO("try to enable RMA");
            prov_hints->caps |= FI_RMA | FI_READ | FI_WRITE | FI_REMOTE_READ | FI_REMOTE_WRITE;
            prov_hints->domain_attr->mr_mode = FI_MR_UNSPEC;
            // TODO:
            //hints->domain_attr->mr_mode = FI_MR_ALLOCATED | FI_MR_PROV_KEY | FI_MR_VIRT_ADDR | FI_MR_LOCAL | FI_MR_BASIC;
        }

        ret = fi_getinfo(fi_version, NULL, NULL, 0ULL, prov_hints, &providers);
        if (ret || !providers) {
            if (attr->enable_rma) {
                attr->enable_rma = 0;
                LOG_INFO("try without RMA");
                prov_hints->caps = FI_TAGGED;
                prov_hints->domain_attr->mr_mode = FI_MR_UNSPEC;
                ret = fi_getinfo(fi_version, NULL, NULL, 0ULL, prov_hints, &providers);
                if (ret || !providers) {
                    LOG_ERROR("fi_getinfo error (rma fallback), prov_idx ", idx);
                    goto err;
                }
            }
            else {
                LOG_ERROR("fi_getinfo error (main path), prov_idx ", idx);
                goto err;
            }
        }

        /* use first provider from the list of providers */
        prov->info = fi_dupinfo(providers);
        struct fi_info* prov_info;
        prov_info = prov->info;
        if (!prov_info) {
            LOG_ERROR("fi_dupinfo error");
            goto err;
        }

        fi_freeinfo(providers);
        providers = NULL;

        attr->max_order_waw_size =
            (idx == 0) ? prov_info->ep_attr->max_order_waw_size
                       : MIN(attr->max_order_waw_size, prov_info->ep_attr->max_order_waw_size);

        prov->max_msg_size = prov_info->ep_attr->max_msg_size;

        if (coord->global_idx == 0) {
            LOG_INFO("provider: ", prov_info->fabric_attr->prov_name);
            LOG_INFO("  mr_mode: ", prov_info->domain_attr->mr_mode);
            LOG_INFO("  threading: ", prov_info->domain_attr->threading);
            LOG_INFO("  tx_ctx_cnt: ", prov_info->domain_attr->tx_ctx_cnt);
            LOG_INFO("  max_ep_tx_ctx: ", prov_info->domain_attr->max_ep_tx_ctx);
            LOG_INFO("  max_msg_size: ", prov_info->ep_attr->max_msg_size);
        }

        ATL_OFI_CALL(fi_fabric(prov_info->fabric_attr, &prov->fabric, NULL), ret, goto err);

        ATL_OFI_CALL(fi_domain(prov->fabric, prov_info, &prov->domain, NULL), ret, goto err);

        av_attr.type = FI_AV_TABLE;
        av_attr.rx_ctx_bits = prov->rx_ctx_bits = (int)ceil(log2(prov_hints->ep_attr->rx_ctx_cnt));

        ATL_OFI_CALL(fi_av_open(prov->domain, &av_attr, &prov->av, NULL), ret, goto err);

        if (prov_info->domain_attr->max_ep_tx_ctx > 1) {
            ATL_OFI_CALL(fi_scalable_ep(prov->domain, prov_info, &prov->sep, NULL), ret, goto err);
            ATL_OFI_CALL(fi_scalable_ep_bind(prov->sep, &prov->av->fid, 0), ret, goto err);
        }

        prov->eps = (atl_ofi_prov_ep_t*)calloc(1, sizeof(atl_ofi_prov_ep_t) * attr->ep_count);
        if (!prov->eps) {
            LOG_ERROR("can't allocate prov->eps");
            goto err;
        }

        for (ep_idx = 0; ep_idx < attr->ep_count; ep_idx++) {
            ret = atl_ofi_prov_ep_init(prov, ep_idx);
            if (ret) {
                LOG_ERROR("atl_ofi_prov_ep_init error");
                goto err;
            }
        }

        if (prov->sep) {
            fi_enable(prov->sep);
        }

        ret = atl_ofi_prov_eps_connect(ofi_ctx, idx, pmi);
        if (ret) {
            LOG_ERROR("atl_ofi_prov_eps_connect error, prov_idx ", idx);
            goto err;
        }

        fi_freeinfo(prov_hints);
        prov_hints = NULL;
    } /* prov loop */

    for (ep_idx = 0; ep_idx < attr->ep_count; ep_idx++) {
        atl_ofi_ep_t* ofi_ep;
        ofi_ep = (atl_ofi_ep_t*)calloc(1, sizeof(atl_ofi_ep_t));
        if (!ofi_ep) {
            LOG_ERROR("can't alloc ofi_ep, idx ", ep_idx);
            goto err;
        }

        atl_ep_t* ep;
        ep = &(ofi_ep->ep);
        ep->idx = ep_idx;
        ep->ctx = ctx;
        ep->p2p_ops = &atl_ofi_ep_p2p_ops;
        ep->coll_ops = &atl_ofi_ep_coll_ops;
        ep->rma_ops = &atl_ofi_ep_rma_ops;
        ep->comp_ops = &atl_ofi_ep_comp_ops;

        ctx->eps[ep_idx] = ep;
    }

    pmi->pmrt_barrier();

    char* max_retry_count_env;
    max_retry_count_env = getenv(ATL_OFI_MAX_RETRY_COUNT_ENV);
    if (max_retry_count_env) {
        ofi_ctx->max_retry_count = safe_c_strtol(max_retry_count_env, NULL, 10);
    }
    else {
        ofi_ctx->max_retry_count = ATL_OFI_MAX_RETRY_COUNT;
    }

    if ((coord->global_count == coord->local_count) && (coord->global_count <= 4)) {
        ofi_ctx->progress_mode = ATL_PROGRESS_CHECK;
    }
    else {
        ofi_ctx->progress_mode = ATL_PROGRESS_POLL;
    }

    char* progress_mode_env;
    progress_mode_env = getenv(ATL_PROGRESS_MODE_ENV);
    if (progress_mode_env) {
        ofi_ctx->progress_mode = static_cast<atl_progress_mode_t>(atoi(progress_mode_env));
    }

    if (coord->global_idx == 0) {
        LOG_INFO("atl-ofi-ctx:");
        LOG_INFO("  new ctx_count: ", global_data.ctx_count);
        LOG_INFO("  max_retry_count: ", ofi_ctx->max_retry_count);
        LOG_INFO("  progress_mode: ", ofi_ctx->progress_mode);
    }

    *out_ctx = ctx;

    fi_freeinfo(base_hints);
    base_hints = NULL;

    return ATL_STATUS_SUCCESS;

err:
    LOG_ERROR("can't find suitable provider");

    if (providers) {
        fi_freeinfo(providers);
    }

    if (base_hints) {
        fi_freeinfo(base_hints);
    }

    if (prov_hints) {
        fi_freeinfo(prov_hints);
    }

    if (ctx != NULL)
        atl_ofi_finalize(ctx);

    return RET2ATL(ret);
}

atl_status_t atl_ofi_main_addr_reserve(char* main_addr) {
    return ATL_STATUS_UNSUPPORTED;
}
