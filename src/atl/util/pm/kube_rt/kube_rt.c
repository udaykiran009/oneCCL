#include "pm_rt_codec.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include <kube.h>

#include "pm_rt.h"

#define KUBE_RT_KEY_FORMAT "%s-%zu-%zu"

typedef struct kube_pm_rt_context {
    pm_rt_desc_t pmrt_desc;
    struct {
        size_t initialized;
        size_t ref_cnt;
        size_t max_keylen;
        size_t max_vallen;
        char *key_storage;
        char *val_storage;
        char *kvsname;
    } kubert_main;
} kube_pm_context_t;

/* Ensures that this is allocated/initialized only once per process */
static kube_pm_context_t kube_ctx_singleton;

static void kube_finalize(pm_rt_desc_t *pmrt_desc)
{
    kube_pm_context_t *ctx =
        container_of(pmrt_desc, kube_pm_context_t, pmrt_desc);
    if (!ctx->kubert_main.initialized)
        return;

    if (--ctx->kubert_main.ref_cnt)
        return;

    free(ctx->kubert_main.kvsname);
    free(ctx->kubert_main.key_storage);
    free(ctx->kubert_main.val_storage);

    KUBE_Finalize();

    memset(ctx, 0, sizeof(*ctx));
}

static void kube_barrier(pm_rt_desc_t *pmrt_desc)
{
    kube_pm_context_t *ctx =
        container_of(pmrt_desc, kube_pm_context_t, pmrt_desc);

    if (!ctx->kubert_main.initialized)
        return;

    KUBE_Barrier();
}

static atl_status_t
kube_kvs_put(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, const void *kvs_val, size_t kvs_val_len)
{
    int ret;
    kube_pm_context_t *ctx =
        container_of(pmrt_desc, kube_pm_context_t, pmrt_desc);

    if (!ctx->kubert_main.initialized)
        return atl_status_failure;

    if (kvs_val_len > ctx->kubert_main.max_vallen)
        return atl_status_failure;

    ret = snprintf(ctx->kubert_main.key_storage, ctx->kubert_main.max_keylen,
                   KUBE_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return atl_status_failure;

    ret = encode(kvs_val, kvs_val_len, ctx->kubert_main.val_storage,
                 ctx->kubert_main.max_vallen);
    if (ret)
        return atl_status_failure;

    ret = KUBE_KVS_Put(ctx->kubert_main.kvsname,
                      ctx->kubert_main.key_storage,
                      ctx->kubert_main.val_storage);
    if (ret != KUBE_SUCCESS)
        return atl_status_failure;

    ret = KUBE_KVS_Commit(ctx->kubert_main.kvsname);
    if (ret != KUBE_SUCCESS)
        return atl_status_failure;

    return atl_status_success;
}

static atl_status_t
kube_kvs_get(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, void *kvs_val, size_t kvs_val_len)
{
    int ret;
    kube_pm_context_t *ctx =
        container_of(pmrt_desc, kube_pm_context_t, pmrt_desc);

    if (!ctx->kubert_main.initialized)
        return atl_status_failure;

    ret = snprintf(ctx->kubert_main.key_storage, ctx->kubert_main.max_keylen,
                   KUBE_RT_KEY_FORMAT, kvs_key, proc_idx, ep_idx);
    if (ret < 0)
        return atl_status_failure;

    ret = KUBE_KVS_Get(ctx->kubert_main.kvsname, ctx->kubert_main.key_storage,
                      ctx->kubert_main.val_storage,
		      ctx->kubert_main.max_vallen);
    if (ret != KUBE_SUCCESS)
        return atl_status_failure;

    ret = decode(ctx->kubert_main.val_storage, kvs_val, kvs_val_len);
    if (ret)
        return atl_status_failure;

    return atl_status_success;
}

static atl_status_t
kube_update(size_t *proc_idx, size_t *proc_count)
{
    int ret;
    ret = KUBE_Update();
    if (ret != KUBE_SUCCESS)
        goto err_kube;

    ret = KUBE_Get_size(proc_count);
    if (ret != KUBE_SUCCESS)
        goto err_kube;

    ret = KUBE_Get_rank(proc_idx);
    if (ret != KUBE_SUCCESS)
        goto err_kube;

    return atl_status_success;

err_kube:
    KUBE_Finalize();
    return atl_status_failure;
}

pm_rt_ops_t kube_ops = {
    .finalize = kube_finalize,
    .barrier  = kube_barrier,
    .update   = kube_update,
};

pm_rt_kvs_ops_t kube_kvs_ops = {
    .put = kube_kvs_put,
    .get = kube_kvs_get,
};

atl_status_t kube_init(size_t *proc_idx, size_t *proc_count, pm_rt_desc_t **pmrt_desc)
{
    int ret;
    size_t max_kvsnamelen;

    if (kube_ctx_singleton.kubert_main.initialized) {
        KUBE_Get_size(proc_idx);
        KUBE_Get_rank(proc_count);
        *pmrt_desc = &kube_ctx_singleton.pmrt_desc;
        kube_ctx_singleton.kubert_main.ref_cnt++;
        return atl_status_success;
    }

    ret = KUBE_Init();
    if (ret != KUBE_SUCCESS)
        return atl_status_failure;

    ret = KUBE_Update();
    if (ret != KUBE_SUCCESS)
        return atl_status_failure;

    ret = KUBE_Get_size(proc_count);
    if (ret != KUBE_SUCCESS)
        goto err_kube;
    ret = KUBE_Get_rank(proc_idx);
    if (ret != KUBE_SUCCESS)
        goto err_kube;

    ret = KUBE_KVS_Get_name_length_max(&max_kvsnamelen);
    if (ret != KUBE_SUCCESS)
        goto err_kube;

    kube_ctx_singleton.kubert_main.kvsname = calloc(1, max_kvsnamelen);
    if (!kube_ctx_singleton.kubert_main.kvsname)
        goto err_kube;

    ret = KUBE_KVS_Get_my_name(kube_ctx_singleton.kubert_main.kvsname,
                              max_kvsnamelen);
    if (ret != KUBE_SUCCESS)
        goto err_alloc_key;

    ret = KUBE_KVS_Get_key_length_max(&kube_ctx_singleton.kubert_main.max_keylen);
    if (ret != KUBE_SUCCESS)
        goto err_alloc_key;

    kube_ctx_singleton.kubert_main.key_storage =
        (char *)calloc(1, kube_ctx_singleton.kubert_main.max_keylen);
    if (!kube_ctx_singleton.kubert_main.key_storage)
        goto err_alloc_key;

    ret = KUBE_KVS_Get_value_length_max(&kube_ctx_singleton.kubert_main.max_vallen);
    if (ret != KUBE_SUCCESS)
        goto err_alloc_val;

    kube_ctx_singleton.kubert_main.val_storage =
        (char *)calloc(1, kube_ctx_singleton.kubert_main.max_vallen);
    if (!kube_ctx_singleton.kubert_main.val_storage)
        goto err_alloc_val;

    kube_ctx_singleton.kubert_main.initialized = 1;
    kube_ctx_singleton.kubert_main.ref_cnt = 1;
    kube_ctx_singleton.pmrt_desc.ops = &kube_ops;
    kube_ctx_singleton.pmrt_desc.kvs_ops = &kube_kvs_ops;
    *pmrt_desc = &kube_ctx_singleton.pmrt_desc;

    return atl_status_success;
err_alloc_val:
    free(kube_ctx_singleton.kubert_main.key_storage);
err_alloc_key:
    free(kube_ctx_singleton.kubert_main.kvsname);
err_kube:
    KUBE_Finalize();
    return atl_status_failure;
}

atl_status_t kube_set_framework_function(update_checker_f user_checker)
{
    int ret = KUBE_set_framework_function(user_checker);

    if (ret)
        return atl_status_failure;

    return atl_status_success;
}
