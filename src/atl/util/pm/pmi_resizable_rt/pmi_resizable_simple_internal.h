#include <list>
#include <map>
#include <memory>
#include <vector>

#include "atl/atl_def.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/internal_kvs.h"
#include "util/pm/pmi_resizable_rt/pmi_resizable/helper.hpp"
#include "atl/util/pm/pm_rt.h"

#define PMIR_SUCCESS                0
#define PMIR_FAIL                   -1
#define PMIR_ERR_INIT               1
#define PMIR_ERR_NOMEM              2
#define PMIR_ERR_INVALID_ARG        3
#define PMIR_ERR_INVALID_KEY        4
#define PMIR_ERR_INVALID_KEY_LENGTH 5
#define PMIR_ERR_INVALID_VAL        6
#define PMIR_ERR_INVALID_VAL_LENGTH 7
#define PMIR_ERR_INVALID_LENGTH     8
#define PMIR_ERR_INVALID_NUM_ARGS   9
#define PMIR_ERR_INVALID_ARGS       10
#define PMIR_ERR_INVALID_NUM_PARSED 11
#define PMIR_ERR_INVALID_KEYVALP    12
#define PMIR_ERR_INVALID_SIZE       13

class pmi_resizable_simple_internal final : public ipmi {
public:
    pmi_resizable_simple_internal() = delete;
    pmi_resizable_simple_internal(int total_rank_count,
                                  const std::vector<int>& ranks,
                                  std::shared_ptr<internal_kvs> k,
                                  const char* main_addr = nullptr);

    ~pmi_resizable_simple_internal() override;

    int is_pm_resize_enabled() override;

    atl_status_t pmrt_main_addr_reserve(char* main_addr) override;

    atl_status_t pmrt_set_resize_function(atl_resize_fn_t resize_fn) override;

    atl_status_t pmrt_update() override;

    atl_status_t pmrt_wait_notification() override;

    void pmrt_barrier() override;

    atl_status_t pmrt_kvs_put(char* kvs_key,
                              int proc_idx,
                              const void* kvs_val,
                              size_t kvs_val_len) override;

    atl_status_t pmrt_kvs_get(char* kvs_key,
                              int proc_idx,
                              void* kvs_val,
                              size_t kvs_val_len) override;

    int get_size() override;

    int get_rank() override;

    size_t get_local_thread_idx() override;

    size_t get_local_kvs_id() override;

    void set_local_kvs_id(size_t local_kvs_id) override;

    size_t get_threads_per_process() override;

    size_t get_ranks_per_process() override;

    void pmrt_finalize() override;

private:
    bool is_finalized{ false };
    atl_status_t pmrt_init(const char* main_addr = nullptr);

    int kvs_set_value(const char* kvs_name, const char* key, const char* value);
    int kvs_get_value(const char* kvs_name, const char* key, char* value);

    void pmrt_barrier_full();
    void barrier_full_reg();
    void barrier_reg();
    void registration();

    int proc_count = 0;
    int rank = 0;
    int proc_rank_count = 0;
    int threads_count = 0;
    int thread_num = 0;

    int total_rank_count;

    std::vector<int> ranks;
    std::shared_ptr<internal_kvs> k;
    size_t max_keylen;
    size_t max_vallen;
    char* val_storage = nullptr;
    size_t local_id;
    size_t kvs_get_timeout = 60; /* in seconds */
};
