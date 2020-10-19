#include <memory>

#include "atl/atl_def.h"
#include "atl/util/pm/pm_rt.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/ikvs_wrapper.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/helper.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/pmi_listener.hpp"

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

typedef enum {
    KVS_RA_WAIT = 0,
    KVS_RA_RUN = 1,
    KVS_RA_FINALIZE = 2,
} kvs_resize_action_t;
typedef kvs_resize_action_t (*pmir_resize_fn_t)(size_t comm_size);

class helper;
class pmi_resizable final : public ipmi {
public:
    pmi_resizable() = delete;
    explicit pmi_resizable(std::shared_ptr<ikvs_wrapper> k, const char* main_addr = nullptr) {
        h = std::shared_ptr<helper>(new helper(k));
        //TODO: move it in one func
        pmrt_init(main_addr);
    }

    ~pmi_resizable() override;

    int is_pm_resize_enabled() override;

    atl_status_t pmrt_main_addr_reserve(char* main_addr) override;

    atl_status_t pmrt_set_resize_function(atl_resize_fn_t resize_fn) override;

    atl_status_t pmrt_update() override;

    atl_status_t pmrt_wait_notification() override;

    void pmrt_barrier() override;

    atl_status_t pmrt_kvs_put(char* kvs_key,
                              size_t proc_idx,
                              const void* kvs_val,
                              size_t kvs_val_len) override;

    atl_status_t pmrt_kvs_get(char* kvs_key,
                              size_t proc_idx,
                              void* kvs_val,
                              size_t kvs_val_len) override;

    void Hard_finilize(int sig);

    size_t get_rank() override;

    size_t get_size() override;

    size_t get_local_thread_idx() override;

    size_t get_local_kvs_id() override;

    void set_local_kvs_id(size_t local_kvs_id) override;

    size_t get_threads_per_process() override {
        return 1;
    }

    size_t get_ranks_per_process() override {
        return 1;
    }
    void pmrt_finalize() override;

private:
    bool is_finalized{ false };
    atl_status_t pmrt_init(const char* main_addr = nullptr);
    /*Was in API ->*/
    int PMIR_Main_Addr_Reserve(char* main_addr);

    int PMIR_Init(const char* main_addr);

    int PMIR_Finalize(void);

    int PMIR_Get_size(size_t* size);

    int PMIR_Get_rank(size_t* rank);

    int PMIR_KVS_Get_my_name(char* kvs_name, size_t length);

    int PMIR_KVS_Get_name_length_max(size_t* length);

    int PMIR_Barrier(void);

    int PMIR_Update(void);

    int PMIR_KVS_Get_key_length_max(size_t* length);

    int PMIR_KVS_Get_value_length_max(size_t* length);

    int PMIR_KVS_Put(const char* kvs_name, const char* key, const char* value);

    int PMIR_KVS_Commit(const char* kvs_name);

    int PMIR_KVS_Get(const char* kvs_name, const char* key, char* value, size_t length);

    int PMIR_set_resize_function(pmir_resize_fn_t resize_fn);

    int PMIR_Wait_notification(void);
    /* <- Was in API*/
    kvs_resize_action_t default_checker(size_t comm_size);
    kvs_resize_action_t call_resize_fn(size_t comm_size);
    size_t rank;
    size_t size;

    pmir_resize_fn_t resize_function = nullptr;
    std::shared_ptr<helper> h;
    pmi_listener listener;
    bool initialized = false;
    size_t max_keylen{};
    size_t max_vallen{};
    char* key_storage = nullptr;
    char* val_storage = nullptr;
    char* kvsname = nullptr;
};