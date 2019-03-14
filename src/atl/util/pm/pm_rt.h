#ifndef PM_RT_H
#define PM_RT_H

#include "atl.h"

#include <pmi.h>

typedef struct pm_rt_desc pm_rt_desc_t;

/* PMI RT */
atl_status_t pmirt_init(size_t *proc_idx, size_t *procs_num, pm_rt_desc_t **pmrt_desc);
atl_status_t kube_init(size_t *proc_idx, size_t *procs_num, pm_rt_desc_t **pmrt_desc);
atl_status_t kube_set_framework_function(update_checker_f user_checker);

typedef enum pm_rt_type {
    PM_RT_PMI,
    PM_RT_KUBE,
} pm_rt_type_t;

typedef struct pm_rt_ops {
    void (*finalize)(pm_rt_desc_t *pmrt_desc);
    void (*barrier)(pm_rt_desc_t *pmrt_desc);
    atl_status_t (*update)(size_t *proc_idx, size_t *proc_count);
} pm_rt_ops_t;

typedef struct pm_rt_kvs_ops {
    atl_status_t (*put)(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                         size_t ep_idx, const void *kvs_val, size_t kvs_val_len);
    atl_status_t (*get)(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
                         size_t ep_idx, void *kvs_val, size_t kvs_val_len);
} pm_rt_kvs_ops_t;

struct pm_rt_desc {
    pm_rt_ops_t *ops;
    pm_rt_kvs_ops_t *kvs_ops;
};

//TODO: Add customize
static pm_rt_type_t type = PM_RT_PMI;
//static pm_rt_type_t type = PM_RT_KUBE;

static inline atl_status_t
pmrt_init(size_t *proc_idx, size_t *procs_num, pm_rt_desc_t **pmrt_desc)
{
    switch (type) {
    case PM_RT_PMI:
        return pmirt_init(proc_idx, procs_num, pmrt_desc);
    case PM_RT_KUBE:
        return kube_init(proc_idx, procs_num, pmrt_desc);
    }
    return atl_status_failure;
}
static inline atl_status_t
pmrt_set_framework_function(update_checker_f user_checker)
{
    switch (type) {
    case PM_RT_KUBE:
        return kube_set_framework_function(user_checker);
    default:
        return atl_status_success;
    }
}
static inline atl_status_t pmrt_update(size_t *proc_idx, size_t *proc_count, pm_rt_desc_t *pmrt_desc)
{
    return pmrt_desc->ops->update(proc_idx, proc_count);
}
static inline void pmrt_finalize(pm_rt_desc_t *pmrt_desc)
{
    pmrt_desc->ops->finalize(pmrt_desc);
}
static inline void pmrt_barrier(pm_rt_desc_t *pmrt_desc)
{
    pmrt_desc->ops->barrier(pmrt_desc);
}
static inline atl_status_t
pmrt_kvs_put(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, const void *kvs_val, size_t kvs_val_len)
{
    return pmrt_desc->kvs_ops->put(pmrt_desc, kvs_key, proc_idx,
                                   ep_idx, kvs_val, kvs_val_len);
}

static inline atl_status_t
pmrt_kvs_get(pm_rt_desc_t *pmrt_desc, char *kvs_key, size_t proc_idx,
             size_t ep_idx, void *kvs_val, size_t kvs_val_len)
{
    return pmrt_desc->kvs_ops->get(pmrt_desc, kvs_key, proc_idx,
                                   ep_idx, kvs_val, kvs_val_len);
}

#endif /* PM_RT_H */
