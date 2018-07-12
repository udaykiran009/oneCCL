#ifndef PM_RT_H
#define PM_RT_H

#include "atl.h"

#include <pmi.h>

typedef struct pm_rt_desc pm_rt_desc_t;

/* PMI RT */
atl_ret_val_t pmirt_init(size_t *proc_idx, size_t *procs_num, pm_rt_desc_t **pmrt_desc);

typedef enum pm_rt_type {
    PM_RT_PMI,
} pm_rt_type_t;

typedef struct pm_rt_ops {
    void (*finalize)(pm_rt_desc_t *pmrt_desc);
    void (*barrier)(pm_rt_desc_t *pmrt_desc);
} pm_rt_ops_t;

typedef struct pm_rt_kvs_ops {
    atl_ret_val_t (*put)(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                         size_t ep_idx, const void *kvs_val, size_t kvs_val_len);
    atl_ret_val_t (*get)(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                         size_t ep_idx, void *kvs_val, size_t kvs_val_len);
} pm_rt_kvs_ops_t;

struct pm_rt_desc {
    pm_rt_ops_t *ops;
    pm_rt_kvs_ops_t *kvs_ops;
};

static inline atl_ret_val_t
pmrt_init(size_t *proc_idx, size_t *procs_num, pm_rt_desc_t **pmrt_desc)
{
    pm_rt_type_t type = PM_RT_PMI;

    switch (type) {
    case PM_RT_PMI:
        return pmirt_init(proc_idx, procs_num, pmrt_desc);
    }
    return ATL_FAILURE;
}
static inline void pmrt_finalize(pm_rt_desc_t *pmrt_desc)
{
    pmrt_desc->ops->finalize(pmrt_desc);
}
static inline void pmrt_barrier(pm_rt_desc_t *pmrt_desc)
{
    pmrt_desc->ops->barrier(pmrt_desc);
}
static inline atl_ret_val_t
pmrt_kvs_put(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, const void *kvs_val, size_t kvs_val_len)
{
    return pmrt_desc->kvs_ops->put(pmrt_desc, kvs_key, proc_idx,
                                   ep_idx, kvs_val, kvs_val_len);
}

static inline atl_ret_val_t
pmrt_kvs_get(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, void *kvs_val, size_t kvs_val_len)
{
    return pmrt_desc->kvs_ops->get(pmrt_desc, kvs_key, proc_idx,
                                   ep_idx, kvs_val, kvs_val_len);
}

#endif /* PM_RT_H */
