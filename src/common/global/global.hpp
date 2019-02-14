#pragma once

#include "common/comm/comm.hpp"
#include "common/utils/utils.hpp"
#include "exec/exec.hpp"
#include "out_of_order/ooo_match.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/sched_cache.hpp"

#include <memory>

//todo: rework MLSL_LOG(ERROR, ..) to not throw
#define COMMON_CATCH_BLOCK()                                        \
catch (mlsl::mlsl_error& mlsl_e)                                    \
{                                                                   \
    MLSL_LOG(INFO, "mlsl intrenal error: %s", mlsl_e.what());       \
    return mlsl_status_invalid_arguments;                           \
}                                                                   \
catch (std::runtime_error& e)                                       \
{                                                                   \
    MLSL_LOG(INFO, "error: %s", e.what());                          \
    return mlsl_status_runtime_error;                               \
}                                                                   \
catch (...)                                                         \
{                                                                   \
    MLSL_LOG(INFO, "general error");                                \
    return mlsl_status_runtime_error;                               \
}                                                                   \


class comm_id_storage;

struct mlsl_global_data
{
    std::shared_ptr<mlsl_comm> comm;
    std::unique_ptr<comm_id_storage> comm_ids;
    std::unique_ptr<mlsl_executor> executor;
    mlsl_sched_cache* sched_cache;
    mlsl_parallelizer* parallelizer;
    mlsl_coll_attr_t* default_coll_attr;
    std::unique_ptr<out_of_order::ooo_match> ooo_handler;
} __attribute__ ((aligned (CACHELINE_SIZE)));

extern mlsl_global_data global_data;

#ifdef ENABLE_DEBUG
#include <atomic>

extern std::atomic_size_t allocations_count;
extern std::atomic_size_t deallocations_count;
#endif