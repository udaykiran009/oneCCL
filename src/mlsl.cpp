#include "mlsl.hpp"
#include "exec/worker.hpp"
#include "exec/exec.hpp"
#include "common/datatype/datatype.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/sched_cache.hpp"
#include "sched/sched_queue.hpp"
#include "out_of_order/ooo_match.hpp"

mlsl_status_t mlsl_init()
{
    mlsl_env_parse();
    mlsl_env_print();
    mlsl_datatype_init();
    mlsl_sched_cache_create(&global_sched_cache);
    mlsl_parallelizer_create(env_data.worker_count, &global_parallelizer);
    size_t min_priority, max_priority;
    mlsl_get_priority_range(&min_priority, &max_priority);

    global_data.executor = std::unique_ptr<mlsl_executor>(new mlsl_executor(env_data.worker_count,
                                                                            max_priority - min_priority + 1,
                                                                            env_data.out_of_order_support != 0,
                                                                            env_data.worker_affinity,
                                                                            env_data.priority_mode));

    mlsl_comm_create_internal(global_data.executor->proc_idx, global_data.executor->proc_count,
                              &global_comm, rank_to_global_rank_map{});

    mlsl_coll_create_attr(&default_coll_attr);
    default_coll_attr->to_cache = true;

    global_data.sched_cache = global_sched_cache;
    global_data.parallelizer = global_parallelizer;
    global_data.comm = global_comm;
    global_data.default_coll_attr = default_coll_attr;

    if (env_data.out_of_order_support)
    {
        global_data.ooo_handler =
            std::unique_ptr<out_of_order::ooo_match>{new out_of_order::ooo_match(global_data.executor.get())};
    }

    return mlsl_status_success;
}

mlsl_status_t mlsl_finalize()
{
    global_data.ooo_handler.reset();
    global_data.executor.reset();

    if (global_data.comm)
        mlsl_comm_free(global_data.comm);
    if (global_data.parallelizer)
        mlsl_parallelizer_free(global_data.parallelizer);
    if (global_data.sched_cache)
        mlsl_sched_cache_free(global_data.sched_cache);
    if (global_data.default_coll_attr)
        mlsl_coll_free_attr(global_data.default_coll_attr);
    mlsl_env_free();

    return mlsl_status_success;
}

size_t mlsl_get_comm_rank(mlsl_comm_t comm)
{
    return comm ? comm->rank : global_data.comm->rank;
}

size_t mlsl_get_comm_size(mlsl_comm_t comm)
{
    return comm ? comm->size : global_data.comm->size;
}

mlsl_status_t mlsl_wait(mlsl_request *req)
{
    if (!req)
        return mlsl_status_success;

    global_data.executor->wait(req);

    mlsl_sched *sched = req->sched;
    MLSL_ASSERT(sched);

    if (!sched->coll_attr.to_cache)
        mlsl_sched_free(sched);

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_test(mlsl_request *req, int *is_completed)
{
    if (!req)
    {
        if (is_completed) *is_completed = 1;
        return mlsl_status_success;
    }

    bool completed = global_data.executor->test(req);

    if (completed)
    {
        mlsl_sched *sched = req->sched;
        MLSL_ASSERTP(sched);

        if (!sched->coll_attr.to_cache)
            mlsl_sched_free(sched);
    }

    *is_completed = static_cast<int>(completed);

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_get_priority_range(size_t *min_priority, size_t *max_priority)
{
    if (min_priority) *min_priority = 0;
    if (max_priority) *max_priority = (MLSL_SCHED_QUEUE_MAX_BINS - 1);
    if (min_priority && max_priority)
        MLSL_ASSERTP(*min_priority <= *max_priority);
    return mlsl_status_success;
}
