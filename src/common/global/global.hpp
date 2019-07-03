#pragma once

#include "iccl.h"
#include "iccl_types.hpp"
#include "common/utils/utils.hpp"

#include <memory>
#include <thread>

#define COMMON_CATCH_BLOCK()                                \
catch (iccl::iccl_error& iccl_e)                            \
{                                                           \
    LOG_ERROR("iccl intrenal error: ", iccl_e.what());      \
    return iccl_status_invalid_arguments;                   \
}                                                           \
catch (std::exception& e)                                   \
{                                                           \
    LOG_ERROR("error: ", e.what());                         \
    return iccl_status_runtime_error;                       \
}                                                           \
catch (...)                                                 \
{                                                           \
    LOG_ERROR("general error");                             \
    return iccl_status_runtime_error;                       \
}                                                           \

class iccl_comm;
class iccl_atl_tag;
class iccl_comm_id_storage;
class iccl_executor;
class iccl_sched_cache;
class iccl_parallelizer;
class double_tree;

namespace out_of_order
{
    class ooo_match;
}
class iccl_fusion_manager;

struct alignas(CACHELINE_SIZE) iccl_global_data
{
    std::shared_ptr<iccl_comm> comm;
    std::unique_ptr<iccl_comm_id_storage> comm_ids;
    std::unique_ptr<iccl_atl_tag> atl_tag;
    std::unique_ptr<iccl_executor> executor;
    std::unique_ptr<iccl_coll_attr_t> default_coll_attr;
    std::unique_ptr<iccl_sched_cache> sched_cache;
    std::unique_ptr<iccl_parallelizer> parallelizer;
    std::unique_ptr<out_of_order::ooo_match> ooo_manager;
    std::unique_ptr<iccl_fusion_manager> fusion_manager;
    static thread_local bool is_worker_thread;
};

extern iccl_global_data global_data;
