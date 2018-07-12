#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_errno.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_tagged.h>

#include "atl.h"

#include "pm_rt.h"

#define CACHELINE_SIZE	64

#ifdef ENABLE_DEBUG
#define ATL_OFI_DEBUG_PRINT(s, ...)                   \
    do {                                              \
        char hoststr[32];                             \
        gethostname(hoststr, sizeof(hoststr));        \
        fprintf(stdout, "(%zu): %s: @ %s:%d:%s() " s, \
	        gettid(), hoststr,                    \
                __FILENAME__, __LINE__,               \
		__func__, ##__VA_ARGS__);             \
        fflush(stdout);                               \
    } while (0)
#else
#define ATL_OFI_DEBUG_PRINT(s, ...)
#endif

#define ATL_OFI_PM_KEY "atl-ofi"

/* OFI returns 0 or -errno */
#define RET2ATL(ret) ((ret) ? ATL_FAILURE : ATL_SUCCESS)

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

static atl_ret_val_t
atl_ofi_comms_connect(atl_ofi_context_t *atl_ofi_context,
                      atl_comm_t **comms, size_t comm_count)
{
    int ret, i, j = 0;
    char *epnames_table;
    size_t epnames_table_len;

    atl_ofi_context->addr_table.ep_num =
        (atl_ofi_context->sep ? 1 : comm_count);

    /* Allocate OFI EP names table that will contain all published names */
    epnames_table_len = FI_NAME_MAX *
                        atl_ofi_context->addr_table.ep_num *
                        atl_ofi_context->proc_count;
    epnames_table = (char *)calloc(1, epnames_table_len);
    if (!epnames_table)
        return ATL_FAILURE;

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
                               /* iterate by comms(EPs) per process */
                               (j * atl_ofi_context->addr_len) +
                               /* iterate by processes */
                               ((i * atl_ofi_context->proc_count) *
                                atl_ofi_context->addr_len), 
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
        ret = ATL_FAILURE;
        goto err_addr_table;
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

static atl_ret_val_t atl_ofi_comms_destroy_conns(atl_ofi_context_t *atl_ofi_context)
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

static atl_ret_val_t
atl_ofi_comm_get_epname(struct fid_ep *ep, atl_ofi_comm_name_t **name)
{
    int ret;

    *name = calloc(1, sizeof(*name));
    if (!*name)
        return ATL_FAILURE;

    ret = fi_getname(&ep->fid, (*name)->addr, &(*name)->len);
    if ((ret != -FI_ETOOSMALL) || ((*name)->len <= 0))
        (*name)->len = FI_NAME_MAX;

    (*name)->addr = calloc(1, (*name)->len);
    if (!(*name)->addr)
        goto err_addr;

    ret = fi_getname(&ep->fid, (*name)->addr, &(*name)->len);
    if (ret)
        goto err_getname;

    return 0;
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

    if (atl_ofi_comm_context->cq)
        fi_close(&atl_ofi_comm_context->cq->fid);
}

static void atl_ofi_comm_destroy(atl_ofi_context_t *atl_ofi_context,
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
    free(atl_ofi_comm_context);
}

static void atl_ofi_comms_destroy(atl_ofi_context_t *atl_ofi_context,
                                  atl_comm_t **comm)
{
    if (atl_ofi_context->sep) {
        atl_ofi_comm_context_t *atl_ofi_comm_context =
            container_of(comm[0], atl_ofi_comm_context_t, atl_comm);
        /* Free memory for one of the COMM contexts, for other - just set to NULL*/
        free(atl_ofi_comm_context->name->addr);
        atl_ofi_comm_context->name->addr = NULL;
        atl_ofi_comm_context->name->len = 0;
        free(atl_ofi_comm_context->name);
        atl_ofi_comms_set_epname(comm, atl_ofi_context->comm_ref_count, NULL);
    }

    do {
        atl_ofi_comm_destroy(atl_ofi_context,
                             comm[atl_ofi_context->comm_ref_count]);
        atl_ofi_context->comm_ref_count--;
    } while (atl_ofi_context->comm_ref_count);

    free(comm);

    if (atl_ofi_context->sep)
        fi_close(&atl_ofi_context->sep->fid);
}

static atl_ret_val_t atl_ofi_finalize(atl_desc_t *atl_desc, atl_comm_t **atl_comms)
{
    int ret;
    atl_ofi_context_t *atl_ofi_context =
        container_of(atl_desc, atl_ofi_context_t, atl_desc);

    ret = atl_ofi_comms_destroy_conns(atl_ofi_context);
    atl_ofi_comms_destroy(atl_ofi_context, atl_comms);
    fi_close(&atl_ofi_context->av->fid);
    fi_close(&atl_ofi_context->domain->fid);
    fi_close(&atl_ofi_context->fabric->fid);
    fi_freeinfo(atl_ofi_context->prov);
    pmrt_finalize(atl_ofi_context->pm_rt);
    free(atl_ofi_context);

    return RET2ATL(ret);
}

static atl_ret_val_t
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
        return ATL_FAILURE;

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

    return 0;
err:
    ofi_comm_ep_destroy(atl_ofi_context, atl_ofi_comm_context);
    return RET2ATL(ret);
}

static atl_pt2pt_ops_t atl_ofi_comm_pt2pt_ops = { 0 };

static atl_ret_val_t
atl_ofi_comm_init(atl_ofi_context_t *atl_ofi_context, atl_comm_attr_t *attr,
                  size_t index, atl_comm_t **comm)
{
    int ret;
    atl_ofi_comm_context_t *atl_ofi_comm_context;

    atl_ofi_comm_context = calloc(1, sizeof(*atl_ofi_comm_context));
    if (!atl_ofi_context)
        return ATL_FAILURE;

    ret = ofi_comm_ep_create(atl_ofi_context, atl_ofi_comm_context, index);
    if (ret)
        goto err_ep;

    atl_ofi_comm_context->idx = index;
    *comm = &atl_ofi_comm_context->atl_comm;
    (*comm)->atl_desc = &atl_ofi_context->atl_desc;
    (*comm)->pt2pt = &atl_ofi_comm_pt2pt_ops;

    return 0;
err_ep:
    free(atl_ofi_comm_context);
    return RET2ATL(ret);
}

static atl_ret_val_t
atl_ofi_comms_init(atl_ofi_context_t *atl_ofi_context, size_t comm_count,
                   atl_comm_attr_t *attr, atl_comm_t ***comms)
{
    int i = 0, ret;

    *comms = calloc(1, sizeof(**comms) * comm_count);
    if (!*comms)
        return ATL_FAILURE;

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

    return 0;
err_comm:
    if (atl_ofi_context->sep)
        fi_close(&atl_ofi_context->sep->fid);
    do {
        atl_ofi_comm_destroy(atl_ofi_context,
                             (*comms)[atl_ofi_context->comm_ref_count]);
        atl_ofi_context->comm_ref_count--;
    } while (atl_ofi_context->comm_ref_count);
err_sep:
    free(*comms);
    return RET2ATL(ret);
}

atl_ops_t atl_ofi_ops = {
    .finalize = atl_ofi_finalize,
};

atl_pt2pt_ops_t atl_ofi_pt2pt_ops;

ATL_OFI_INI
{
    struct fi_info *providers, *hints;
    struct fi_av_attr av_attr = { (enum fi_av_type)0 };
    int fi_version, ret;
    atl_ofi_context_t *atl_ofi_context;

    atl_ofi_context = calloc(1, sizeof(*atl_ofi_context));
    if (!atl_ofi_context)
        return ATL_FAILURE;

    ret = pmrt_init(proc_idx, proc_count, &atl_ofi_context->pm_rt);
    if (ret)
        goto err_pmrt_init;

    atl_ofi_context->proc_idx = *proc_idx;
    atl_ofi_context->proc_count = *proc_count;

    hints = fi_allocinfo();
    if (!hints)
        goto err_hints;
    hints->mode = FI_CONTEXT;
    hints->ep_attr->type = FI_EP_RDM;
    hints->domain_attr->resource_mgmt = FI_RM_ENABLED;
    hints->caps = FI_TAGGED;

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

    return 0;
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
