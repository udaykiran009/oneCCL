#pragma once

#include "mlsl.h"
#include "mlsl_types.hpp"
#include "common/utils/utils.hpp"

#include <memory>
#include <thread>

#define COMMON_CATCH_BLOCK()                                        \
catch (mlsl::mlsl_error& mlsl_e)                                    \
{                                                                   \
    LOG_ERROR("mlsl intrenal error: ", mlsl_e.what());              \
    return mlsl_status_invalid_arguments;                           \
}                                                                   \
catch (std::exception& e)                                           \
{                                                                   \
    LOG_ERROR("error: ", e.what());                                 \
    return mlsl_status_runtime_error;                               \
}                                                                   \
catch (...)                                                         \
{                                                                   \
    LOG_ERROR("general error");                                     \
    return mlsl_status_runtime_error;                               \
}                                                                   \

class mlsl_comm;
class comm_id_storage;
class mlsl_executor;
class mlsl_sched_cache;
class mlsl_parallelizer;
namespace out_of_order
{
    class ooo_match;
}
class mlsl_fusion_manager;

struct alignas(CACHELINE_SIZE) mlsl_global_data
{
    std::shared_ptr<mlsl_comm> comm;
    std::unique_ptr<comm_id_storage> comm_ids;
    std::unique_ptr<mlsl_executor> executor;
    std::unique_ptr<mlsl_coll_attr_t> default_coll_attr;
    std::unique_ptr<mlsl_sched_cache> sched_cache;
    std::unique_ptr<mlsl_parallelizer> parallelizer;
    std::unique_ptr<out_of_order::ooo_match> ooo_manager;
    std::unique_ptr<mlsl_fusion_manager> fusion_manager;
    static thread_local bool is_worker_thread;
};

extern mlsl_global_data global_data;
