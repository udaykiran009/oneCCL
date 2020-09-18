#include "pmi_simple.h"
#include "pmi_rt.c"

int pmi_simple::is_pm_resize_enabled() {
    return false;
}

pmi_simple::pmi_simple() {
    pmirt_init(&rank, &size, &pmrt_desc);
}

atl_status_t pmi_simple::pmrt_main_addr_reserv(char *main_addr) {
    printf("Function main_addr_reserv unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_set_resize_function(atl_resize_fn_t resize_fn) {
    printf("Function set_resize_function unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_update() {
    printf("Function update unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

atl_status_t pmi_simple::pmrt_wait_notification() {
    printf("Function wait_notification unsupported yet for simple pmi\n");
    return ATL_STATUS_FAILURE;
}

void pmi_simple::pmrt_finalize() {
    is_finalized = true;
    pmirt_finalize(pmrt_desc);
}

void pmi_simple::pmrt_barrier() {
    pmirt_barrier(pmrt_desc);
}

atl_status_t pmi_simple::pmrt_kvs_put(char *kvs_key,
                                      size_t proc_idx,
                                      const void *kvs_val,
                                      size_t kvs_val_len) {
    return pmirt_kvs_put(pmrt_desc, kvs_key, proc_idx, kvs_val, kvs_val_len);
}

atl_status_t pmi_simple::pmrt_kvs_get(char *kvs_key,
                                      size_t proc_idx,
                                      void *kvs_val,
                                      size_t kvs_val_len) {
    return pmirt_kvs_get(pmrt_desc, kvs_key, proc_idx, kvs_val, kvs_val_len);
}

size_t pmi_simple::get_rank() {
    return rank;
}

size_t pmi_simple::get_size() {
    return size;
}

size_t pmi_simple::get_thread() {
    return 0;
}
size_t pmi_simple::get_local_kvs_id() {
    return 0;
}
void pmi_simple::set_local_kvs_id(size_t local_kvs_id) {}
pmi_simple::~pmi_simple() {
    if (!is_finalized)
        pmrt_finalize();
}
