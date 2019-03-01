#pragma once

#include "mlsl.h"
#include "mlsl_types.hpp"
#include "common/comm/comm.hpp"
#include "common/utils/utils.hpp"
#include "exec/exec.hpp"
#include "out_of_order/ooo_match.hpp"
#include "fusion/fusion.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"

#include <memory>
#include <thread>

#define COMMON_CATCH_BLOCK()                                        \
catch (mlsl::mlsl_error& mlsl_e)                                    \
{                                                                   \
    MLSL_LOG(ERROR, "mlsl intrenal error: %s", mlsl_e.what());      \
    return mlsl_status_invalid_arguments;                           \
}                                                                   \
catch (std::exception& e)                                           \
{                                                                   \
    MLSL_LOG(ERROR, "error: %s", e.what());                         \
    return mlsl_status_runtime_error;                               \
}                                                                   \
catch (...)                                                         \
{                                                                   \
    MLSL_LOG(ERROR, "general error");                               \
    return mlsl_status_runtime_error;                               \
}                                                                   \

class comm_id_storage;

struct alignas(CACHELINE_SIZE) mlsl_global_data
{
    std::shared_ptr<mlsl_comm> comm;
    std::unique_ptr<comm_id_storage> comm_ids;
    std::unique_ptr<mlsl_executor> executor;
    mlsl_sched_cache* sched_cache;
    mlsl_coll_attr_t* default_coll_attr;
    std::unique_ptr<mlsl_parallelizer> parallelizer;
    std::unique_ptr<out_of_order::ooo_match> ooo_handler;
    std::unique_ptr<mlsl_fusion_manager> fusion_manager;
    static thread_local bool is_worker_thread;
};

extern mlsl_global_data global_data;
