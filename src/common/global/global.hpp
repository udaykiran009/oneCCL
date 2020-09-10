#pragma once

//#include "ccl.h"
#include "coll/algorithms/algorithms_enum.hpp"
#include "comp/bfp16/bfp16_utils.h"
#include "common/env/env.hpp"
#include "common/utils/utils.hpp"

#include <memory>
#include <thread>

#define COMMON_CATCH_BLOCK()                             \
    catch (ccl::ccl_error& ccl_e)                        \
    {                                                    \
        LOG_ERROR("ccl internal error: ", ccl_e.what()); \
        return ccl_status_invalid_arguments;             \
    }                                                    \
    catch (std::exception& e)                            \
    {                                                    \
        LOG_ERROR("error: ", e.what());                  \
        return ccl_status_runtime_error;                 \
    }                                                    \
    catch (...)                                          \
    {                                                    \
        LOG_ERROR("general error");                      \
        return ccl_status_runtime_error;                 \
    }                                                    \

class ccl_comm;
class ccl_stream;
class ccl_atl_tag;
class ccl_comm_id_storage;
class ccl_datatype_storage;
class ccl_executor;
class ccl_sched_cache;
class ccl_parallelizer;
class ccl_fusion_manager;
class ccl_unordered_coll_manager;
class ccl_allreduce_2d_builder;

template<ccl_coll_type... registered_types_id>
class ccl_algorithm_selector_wrapper;

namespace ccl
{

class global_data
{
public:

    global_data(const global_data&) = delete;
    global_data(global_data&&) = delete;

    global_data& operator=(const global_data&) = delete;
    global_data& operator=(global_data&&) = delete;

    ~global_data();

    ccl_status_t init();
    ccl_status_t reset();

    static global_data& get();
    static env_data& env();

    /* public methods to have access from listener thread function */
    void init_resize_dependent_objects();
    void reset_resize_dependent_objects();

    std::unique_ptr<ccl_comm_id_storage> comm_ids;
    std::shared_ptr<ccl_comm> comm;
    std::unique_ptr<ccl_datatype_storage> dtypes;
    std::unique_ptr<ccl_atl_tag> atl_tag;
    std::unique_ptr<ccl_executor> executor;
    std::unique_ptr<ccl_coll_attr_t> default_coll_attr;
    std::unique_ptr<ccl_sched_cache> sched_cache;
    std::unique_ptr<ccl_parallelizer> parallelizer;
    std::unique_ptr<ccl_fusion_manager> fusion_manager;
    std::unique_ptr<ccl_unordered_coll_manager> unordered_coll_manager;
    std::unique_ptr<ccl_algorithm_selector_wrapper<CCL_COLL_LIST>> algorithm_selector;
    std::unique_ptr<ccl_allreduce_2d_builder> allreduce_2d_builder;
    static thread_local bool is_worker_thread;
    bool is_ft_enabled;
    ccl_bfp16_impl_type bfp16_impl_type;

//TODO new_api configure thread wait timeout
    size_t thread_barrier_wait_timeout_sec = 5;


private:
    global_data();

    void init_resize_independent_objects();
    void reset_resize_independent_objects();

    env_data env_object;
};

#define CCL_CHECK_IS_BLOCKED()                            \
  {                                                       \
    do                                                    \
    {                                                     \
        if (unlikely(                                     \
            ccl::global_data::get().executor->is_locked)) \
        {                                                 \
            return ccl_status_blocked_due_to_resize;      \
        }                                                 \
    } while (0);                                          \
  }

} /* namespace ccl */
