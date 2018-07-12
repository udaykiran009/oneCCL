#include "pmi_rt_codec.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <pmi.h>

#include "pm_rt.h"

#define PMI_RT_KEY_FORMAT "%s-%zu-%zu"

typedef struct pmi_pm_rt_context {
    pm_rt_desc_t pmrt_desc;
    struct {
        int initialized;
        int ref_cnt;
        int max_keylen;
        int max_vallen;
        char *key_storage;
        char *val_storage;
        char *kvsname;
    } pmirt_main;
} pmi_pm_context_t;

/* Ensures that this is allocated/initialized only once per process */
static pmi_pm_context_t pmi_ctx_singleton;

static void pmirt_finalize(pm_rt_desc_t *pmrt_desc)
{
    pmi_pm_context_t *ctx =
        container_of(pmrt_desc, pmi_pm_context_t, pmrt_desc);
    if (!ctx->pmirt_main.initialized)
        return;

    if (--ctx->pmirt_main.ref_cnt)
        return;

    free(ctx->pmirt_main.kvsname);
    free(ctx->pmirt_main.key_storage);
    free(ctx->pmirt_main.val_storage);

    PMI_Finalize();

    memset(ctx, 0, sizeof(*ctx));
}

static atl_ret_val_t
pmirt_kvs_put(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
              size_t ep_idx, const void *kvs_val, size_t kvs_val_len)
{
    int ret;
    pmi_pm_context_t *ctx =
        container_of(pmrt_desc, pmi_pm_context_t, pmrt_desc);

    if (!ctx->pmirt_main.initialized)
        return ATL_FAILURE;

    if (kvs_val_len > ctx->pmirt_main.max_vallen)
        return ATL_FAILURE;

    ret = snprintf(ctx->pmirt_main.key_storage, ctx->pmirt_main.max_keylen,
                   PMI_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return ATL_FAILURE;

    ret = encode(kvs_val, kvs_val_len, ctx->pmirt_main.val_storage,
                 ctx->pmirt_main.max_vallen);
    if (ret)
        return ATL_FAILURE;

    ret = PMI_KVS_Put(ctx->pmirt_main.kvsname,
                      ctx->pmirt_main.key_storage,
                      ctx->pmirt_main.val_storage);
    if (ret != PMI_SUCCESS)
        return ATL_FAILURE;

    ret = PMI_KVS_Commit(ctx->pmirt_main.kvsname);
    if (ret != PMI_SUCCESS)
        return ATL_FAILURE;

    return 0;
}

static atl_ret_val_t
pmirt_kvs_get(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
              size_t ep_idx, void *kvs_val, size_t kvs_val_len)
{
    int ret;
    pmi_pm_context_t *ctx =
        container_of(pmrt_desc, pmi_pm_context_t, pmrt_desc);

    if (!ctx->pmirt_main.initialized)
        return ATL_FAILURE;

    ret = snprintf(ctx->pmirt_main.key_storage, ctx->pmirt_main.max_keylen,
                   PMI_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return ATL_FAILURE;

    ret = PMI_KVS_Get(ctx->pmirt_main.kvsname, ctx->pmirt_main.key_storage,
                      ctx->pmirt_main.val_storage,
		      ctx->pmirt_main.max_vallen);
    if (ret != PMI_SUCCESS)
        return ATL_FAILURE;

    ret = decode(ctx->pmirt_main.val_storage, kvs_val, kvs_val_len);
    if (ret)
        return ATL_FAILURE;

    return 0;
}

static void pmirt_barrier(pm_rt_desc_t *pmrt_desc)
{
    pmi_pm_context_t *ctx =
        container_of(pmrt_desc, pmi_pm_context_t, pmrt_desc);

    if (!ctx->pmirt_main.initialized)
        return;

    (void) PMI_Barrier();
}

pm_rt_ops_t ops = {
    .finalize = pmirt_finalize,
    .barrier = pmirt_barrier,
};

pm_rt_kvs_ops_t kvs_ops = {
    .put = pmirt_kvs_put,
    .get = pmirt_kvs_get,
};

atl_ret_val_t pmirt_init(size_t *proc_idx, size_t *proc_count, pm_rt_desc_t **pmrt_desc)
{
    int ret, spawned, max_kvsnamelen;

    if (pmi_ctx_singleton.pmirt_main.initialized) {
        PMI_Get_size((int *)proc_count);
        PMI_Get_rank((int *)proc_idx);
        *pmrt_desc = &pmi_ctx_singleton.pmrt_desc;
        pmi_ctx_singleton.pmirt_main.ref_cnt++;
        return 0;
    }

    ret = PMI_Init(&spawned);
    if (ret != PMI_SUCCESS)
        return ATL_FAILURE;

    ret = PMI_Get_size((int *)proc_count);
    if (ret != PMI_SUCCESS)
        goto err_pmi;

    ret = PMI_Get_rank((int *)proc_idx);
    if (ret != PMI_SUCCESS)
        goto err_pmi;

    ret = PMI_KVS_Get_name_length_max(&max_kvsnamelen);
    if (ret != PMI_SUCCESS)
        goto err_pmi;

    pmi_ctx_singleton.pmirt_main.kvsname = calloc(1, max_kvsnamelen);
    if (!pmi_ctx_singleton.pmirt_main.kvsname)
        goto err_pmi;

    ret = PMI_KVS_Get_my_name(pmi_ctx_singleton.pmirt_main.kvsname,
                              max_kvsnamelen);
    if (ret != PMI_SUCCESS)
        goto err_alloc_key;

    ret = PMI_KVS_Get_key_length_max(&pmi_ctx_singleton.pmirt_main.max_keylen);
    if (ret != PMI_SUCCESS)
        goto err_alloc_key;

    pmi_ctx_singleton.pmirt_main.key_storage =
        (char *)calloc(1, pmi_ctx_singleton.pmirt_main.max_keylen);
    if (!pmi_ctx_singleton.pmirt_main.key_storage)
        goto err_alloc_key;

    ret = PMI_KVS_Get_value_length_max(&pmi_ctx_singleton.pmirt_main.max_vallen);
    if (ret != PMI_SUCCESS)
        goto err_alloc_val;

    pmi_ctx_singleton.pmirt_main.val_storage =
        (char *)calloc(1, pmi_ctx_singleton.pmirt_main.max_vallen);
    if (!pmi_ctx_singleton.pmirt_main.val_storage)
        goto err_alloc_val;

    pmi_ctx_singleton.pmirt_main.initialized = 1;
    pmi_ctx_singleton.pmirt_main.ref_cnt = 1;
    pmi_ctx_singleton.pmrt_desc.ops = &ops;
    pmi_ctx_singleton.pmrt_desc.kvs_ops = &kvs_ops;
    *pmrt_desc = &pmi_ctx_singleton.pmrt_desc;

    return 0;
err_alloc_val:
    free(pmi_ctx_singleton.pmirt_main.key_storage);
err_alloc_key:
    free(pmi_ctx_singleton.pmirt_main.kvsname);
err_pmi:
    PMI_Finalize();
    return ATL_FAILURE;    
}
