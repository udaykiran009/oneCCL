#include "common/log/log.hpp"
#include "pmi_simple.h"
#include "pmi_rt.c"

int pmi_simple::is_pm_resize_enabled() {
    return false;
}

pmi_simple::pmi_simple() {}

atl_status_t pmi_simple::pmrt_init() {
    return pmirt_init(&rank, &size, &pmrt_desc);
}

atl_status_t pmi_simple::pmrt_main_addr_reserve(char *main_addr) {
    LOG_ERROR("Function main_addr_reserv unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_set_resize_function(atl_resize_fn_t resize_fn) {
    LOG_ERROR("Function set_resize_function unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_update() {
    LOG_ERROR("Function update unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_wait_notification() {
    LOG_ERROR("Function wait_notification unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_finalize() {
    is_finalized = true;
    pmirt_finalize(pmrt_desc);
    return ATL_STATUS_SUCCESS;
}

atl_status_t pmi_simple::pmrt_barrier() {
    pmirt_barrier(pmrt_desc);
    return ATL_STATUS_SUCCESS;
}

atl_status_t pmi_simple::pmrt_kvs_put(char *kvs_key,
                                      int proc_idx,
                                      const void *kvs_val,
                                      size_t kvs_val_len) {
    return pmirt_kvs_put(pmrt_desc, kvs_key, proc_idx, kvs_val, kvs_val_len);
}

atl_status_t pmi_simple::pmrt_kvs_get(char *kvs_key,
                                      int proc_idx,
                                      void *kvs_val,
                                      size_t kvs_val_len) {
    return pmirt_kvs_get(pmrt_desc, kvs_key, proc_idx, kvs_val, kvs_val_len);
}

int pmi_simple::get_rank() {
    return rank;
}

int pmi_simple::get_size() {
    return size;
}

size_t pmi_simple::get_local_thread_idx() {
    return 0;
}
atl_status_t pmi_simple::get_local_kvs_id(size_t &res) {
    res = 0;
    return ATL_STATUS_SUCCESS;
}
atl_status_t pmi_simple::set_local_kvs_id(size_t local_kvs_id) {
    return ATL_STATUS_SUCCESS;
}

pmi_simple::~pmi_simple() {
    if (!is_finalized) {
        CCL_THROW_IF_NOT(pmrt_finalize() == ATL_STATUS_SUCCESS, "~pmi_simple: failed");
    }
}
