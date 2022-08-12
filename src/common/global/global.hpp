#pragma once

#include "coll/algorithms/algorithm_utils.hpp"
#include "common/env/env.hpp"

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include "common/global/ze/ze_data.hpp"
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
#include "common/utils/utils.hpp"
#include "hwloc/hwloc_wrapper.hpp"
#include "internal_types.hpp"

#include <memory>
#include <thread>

class ccl_datatype_storage;
class ccl_executor;
class ccl_sched_cache;
class ccl_parallelizer;
class ccl_fusion_manager;

template <ccl_coll_type... registered_types_id>
class ccl_algorithm_selector_wrapper;

namespace ccl {

class buffer_cache;

struct os_information {
    std::string sysname;
    std::string nodename;
    std::string release;
    std::string version;
    std::string machine;

    std::string to_string();
    void fill();
};

class global_data {
public:
    global_data(const global_data&) = delete;
    global_data(global_data&&) = delete;

    global_data& operator=(const global_data&) = delete;
    global_data& operator=(global_data&&) = delete;

    ~global_data();

    ccl::status init();
    ccl::status reset();

    static global_data& get();
    static env_data& env();
    static os_information& get_os_info();

    /* public methods to have access from listener thread function */
    void init_resize_dependent_objects();
    void reset_resize_dependent_objects();

    std::unique_ptr<ccl_datatype_storage> dtypes;
    std::unique_ptr<ccl_executor> executor;
    std::unique_ptr<ccl_sched_cache> sched_cache;
    std::unique_ptr<ccl::buffer_cache> buffer_cache;
    std::unique_ptr<ccl_parallelizer> parallelizer;
    std::unique_ptr<ccl_fusion_manager> fusion_manager;
    std::unique_ptr<ccl_algorithm_selector_wrapper<CCL_COLL_LIST>> algorithm_selector;
    std::unique_ptr<ccl_hwloc_wrapper> hwloc_wrapper;

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    std::unique_ptr<ze::global_data_desc> ze_data;
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

    static thread_local bool is_worker_thread;
    bool is_ft_enabled;

    int get_local_proc_idx() const {
        return local_proc_idx;
    }
    int get_local_proc_count() const {
        return local_proc_count;
    }

    void set_local_coord();

private:
    global_data();

    void init_resize_independent_objects();
    void reset_resize_independent_objects();

    int local_proc_idx;
    int local_proc_count;
    void getenv_local_coord(const char* local_proc_idx_env_name,
                            const char* local_proc_count_env_name);
    env_data env_object;
    os_information os_info;
};

} // namespace ccl
