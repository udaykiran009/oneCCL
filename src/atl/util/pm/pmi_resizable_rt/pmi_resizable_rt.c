#include "pm_rt_codec.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "pmi_resizable/resizable_pmi.h"

#include "pm_rt.h"

#define RESIZABLE_PMI_RT_KEY_FORMAT "%s-%zu-%zu"

typedef struct resizable_pm_rt_context {
    pm_rt_desc_t pmrt_desc;
    struct {
        size_t initialized;
        size_t ref_cnt;
        size_t max_keylen;
        size_t max_vallen;
        char *key_storage;
        char *val_storage;
        char *kvsname;
    } resizablert_main;
} resizable_pm_context_t;

/* Ensures that this is allocated/initialized only once per process */
static resizable_pm_context_t resizable_ctx_singleton;

static void resizable_pmirt_finalize(pm_rt_desc_t *pmrt_desc)
{
    resizable_pm_context_t *ctx =
        container_of(pmrt_desc, resizable_pm_context_t, pmrt_desc);
    if (!ctx->resizablert_main.initialized)
        return;

    if (--ctx->resizablert_main.ref_cnt)
        return;

    free(ctx->resizablert_main.kvsname);
    free(ctx->resizablert_main.key_storage);
    free(ctx->resizablert_main.val_storage);

    PMIR_Finalize();

    memset(ctx, 0, sizeof(*ctx));
}

static void resizable_pmirt_barrier(pm_rt_desc_t *pmrt_desc)
{
    resizable_pm_context_t *ctx =
        container_of(pmrt_desc, resizable_pm_context_t, pmrt_desc);

    if (!ctx->resizablert_main.initialized)
        return;

    PMIR_Barrier();
}

static atl_status_t
resizable_pmirt_kvs_put(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                        size_t ep_idx, const void *kvs_val, size_t kvs_val_len)
{
    int ret;
    resizable_pm_context_t *ctx =
        container_of(pmrt_desc, resizable_pm_context_t, pmrt_desc);

    if (!ctx->resizablert_main.initialized)
        return atl_status_failure;

    if (kvs_val_len > ctx->resizablert_main.max_vallen)
        return atl_status_failure;

    ret = snprintf(ctx->resizablert_main.key_storage, ctx->resizablert_main.max_keylen,
                   RESIZABLE_PMI_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return atl_status_failure;

    ret = encode(kvs_val, kvs_val_len, ctx->resizablert_main.val_storage,
                 ctx->resizablert_main.max_vallen);
    if (ret)
        return atl_status_failure;

    ret = PMIR_KVS_Put(ctx->resizablert_main.kvsname,
                       ctx->resizablert_main.key_storage,
                       ctx->resizablert_main.val_storage);
    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    ret = PMIR_KVS_Commit(ctx->resizablert_main.kvsname);
    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    return atl_status_success;
}

static atl_status_t
resizable_pmirt_kvs_get(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                        size_t ep_idx, void *kvs_val, size_t kvs_val_len)
{
    int ret;
    resizable_pm_context_t *ctx =
        container_of(pmrt_desc, resizable_pm_context_t, pmrt_desc);

    if (!ctx->resizablert_main.initialized)
        return atl_status_failure;

    ret = snprintf(ctx->resizablert_main.key_storage, ctx->resizablert_main.max_keylen,
                   RESIZABLE_PMI_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return atl_status_failure;

    ret = PMIR_KVS_Get(ctx->resizablert_main.kvsname, ctx->resizablert_main.key_storage,
                       ctx->resizablert_main.val_storage,
                       ctx->resizablert_main.max_vallen);
    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    ret = decode(ctx->resizablert_main.val_storage, kvs_val, kvs_val_len);
    if (ret)
        return atl_status_failure;

    return atl_status_success;
}

static atl_status_t
resizable_pmirt_update(size_t *proc_idx, size_t *proc_count)
{
    int ret;
    ret = PMIR_Update();
    if (ret != PMIR_SUCCESS)
        goto err_resizable;

    ret = PMIR_Get_size(proc_count);
    if (ret != PMIR_SUCCESS)
        goto err_resizable;

    ret = PMIR_Get_rank(proc_idx);
    if (ret != PMIR_SUCCESS)
        goto err_resizable;

    return atl_status_success;

err_resizable:
    PMIR_Finalize();
    return atl_status_failure;
}

atl_status_t resizable_pmirt_wait_notification()
{
    int ret;

    ret = PMIR_Wait_notification();

    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    return atl_status_success;
}

pm_rt_ops_t resizable_ops = {
    .finalize = resizable_pmirt_finalize,
    .barrier  = resizable_pmirt_barrier,
    .update   = resizable_pmirt_update,
    .wait_notification = resizable_pmirt_wait_notification,
};

pm_rt_kvs_ops_t resizable_kvs_ops = {
    .put = resizable_pmirt_kvs_put,
    .get = resizable_pmirt_kvs_get,
};

atl_status_t resizable_pmirt_init(size_t *proc_idx, size_t *proc_count, pm_rt_desc_t **pmrt_desc)
{
    int ret;
    size_t max_kvsnamelen;

    if (resizable_ctx_singleton.resizablert_main.initialized) {
        PMIR_Get_size(proc_idx);
        PMIR_Get_rank(proc_count);
        *pmrt_desc = &resizable_ctx_singleton.pmrt_desc;
        resizable_ctx_singleton.resizablert_main.ref_cnt++;
        return atl_status_success;
    }

    ret = PMIR_Init();
    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    ret = PMIR_Update();
    if (ret != PMIR_SUCCESS)
        return atl_status_failure;

    ret = PMIR_Get_size(proc_count);
    if (ret != PMIR_SUCCESS)
        goto err_resizable;
    ret = PMIR_Get_rank(proc_idx);
    if (ret != PMIR_SUCCESS)
        goto err_resizable;

    ret = PMIR_KVS_Get_name_length_max(&max_kvsnamelen);
    if (ret != PMIR_SUCCESS)
        goto err_resizable;

    resizable_ctx_singleton.resizablert_main.kvsname = calloc(1, max_kvsnamelen);
    if (!resizable_ctx_singleton.resizablert_main.kvsname)
        goto err_resizable;

    ret = PMIR_KVS_Get_my_name(resizable_ctx_singleton.resizablert_main.kvsname,
                               max_kvsnamelen);
    if (ret != PMIR_SUCCESS)
        goto err_alloc_key;

    ret = PMIR_KVS_Get_key_length_max(&resizable_ctx_singleton.resizablert_main.max_keylen);
    if (ret != PMIR_SUCCESS)
        goto err_alloc_key;

    resizable_ctx_singleton.resizablert_main.key_storage =
        (char *)calloc(1, resizable_ctx_singleton.resizablert_main.max_keylen);
    if (!resizable_ctx_singleton.resizablert_main.key_storage)
        goto err_alloc_key;

    ret = PMIR_KVS_Get_value_length_max(&resizable_ctx_singleton.resizablert_main.max_vallen);
    if (ret != PMIR_SUCCESS)
        goto err_alloc_val;

    resizable_ctx_singleton.resizablert_main.val_storage =
        (char *)calloc(1, resizable_ctx_singleton.resizablert_main.max_vallen);
    if (!resizable_ctx_singleton.resizablert_main.val_storage)
        goto err_alloc_val;

    resizable_ctx_singleton.resizablert_main.initialized = 1;
    resizable_ctx_singleton.resizablert_main.ref_cnt = 1;
    resizable_ctx_singleton.pmrt_desc.ops = &resizable_ops;
    resizable_ctx_singleton.pmrt_desc.kvs_ops = &resizable_kvs_ops;
    *pmrt_desc = &resizable_ctx_singleton.pmrt_desc;

    return atl_status_success;
err_alloc_val:
    free(resizable_ctx_singleton.resizablert_main.key_storage);
err_alloc_key:
    free(resizable_ctx_singleton.resizablert_main.kvsname);
err_resizable:
    PMIR_Finalize();
    return atl_status_failure;
}

atl_status_t resizable_pmirt_set_resize_function(atl_resize_fn_t resize_fn)
{
    int ret = PMIR_set_resize_function((pmir_resize_fn_t) resize_fn);

    if (ret)
        return atl_status_failure;

    return atl_status_success;
}
