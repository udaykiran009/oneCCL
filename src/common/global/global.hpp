#pragma once

#include "ccl.h"
#include "common/utils/utils.hpp"

#include <memory>
#include <thread>

#define COMMON_CATCH_BLOCK()                             \
    catch (ccl::ccl_error& ccl_e)                        \
    {                                                    \
        LOG_ERROR("ccl intrenal error: ", ccl_e.what()); \
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
class ccl_executor;
class ccl_sched_cache;
class ccl_parallelizer;
class double_tree;
class ccl_fusion_manager;
class ccl_unordered_coll_manager;
class ccl_coll_algorithm_selector;

struct alignas(CACHELINE_SIZE) ccl_global_data
{
    std::shared_ptr<ccl_comm> comm;
    std::unique_ptr<ccl_comm_id_storage> comm_ids;
    std::unique_ptr<ccl_atl_tag> atl_tag;
    std::unique_ptr<ccl_executor> executor;
    std::unique_ptr<ccl_coll_attr_t> default_coll_attr;
    std::unique_ptr<ccl_sched_cache> sched_cache;
    std::unique_ptr<ccl_parallelizer> parallelizer;
    std::unique_ptr<ccl_fusion_manager> fusion_manager;
    std::unique_ptr<ccl_unordered_coll_manager> unordered_coll_manager;
    std::unique_ptr<ccl_coll_algorithm_selector> algorithm_selector;
    static thread_local bool is_worker_thread;
};

extern ccl_global_data global_data;
