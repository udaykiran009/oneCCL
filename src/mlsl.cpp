#include "exec/exec.hpp"
#include "common/global/global.hpp"
#include "sched/sched_queue.hpp"

mlsl_status_t mlsl_init()
{
    try
    {
#ifdef ENABLE_DEBUG
        allocations_count = 0;
        deallocations_count = 0;
#endif

        mlsl_env_parse();
        mlsl_env_print();
        mlsl_datatype_init();
        mlsl_sched_cache_create(&global_sched_cache);
        global_data.parallelizer = std::unique_ptr<mlsl_parallelizer>(new mlsl_parallelizer(env_data.worker_count));

        if (env_data.enable_fusion)
        {
            global_data.fusion_manager =
                std::unique_ptr<mlsl_fusion_manager>(new mlsl_fusion_manager());
        }

        size_t min_priority, max_priority;
        mlsl_get_priority_range(&min_priority, &max_priority);
        global_data.executor = std::unique_ptr<mlsl_executor>(new mlsl_executor(env_data.worker_count,
                                                                                max_priority - min_priority + 1,
                                                                                env_data.out_of_order_support != 0 ||
                                                                                env_data.enable_fusion != 0,
                                                                                env_data.worker_affinity,
                                                                                env_data.priority_mode));

        global_data.comm_ids = std::unique_ptr<comm_id_storage>(new comm_id_storage(mlsl_comm::max_comm_count));

        global_data.comm = std::make_shared<mlsl_comm>(global_data.executor->proc_idx,
                                                       global_data.executor->proc_count,
                                                       std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));

        mlsl_coll_create_attr(&default_coll_attr);
        default_coll_attr->to_cache = true;

        global_data.sched_cache = global_sched_cache;
        global_data.default_coll_attr = default_coll_attr;

        if (env_data.out_of_order_support)
        {
            global_data.ooo_handler =
                std::unique_ptr<out_of_order::ooo_match>(new out_of_order::ooo_match(*global_data.executor, *global_data.comm_ids));
        }

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_finalize()
{
    try
    {
        if (global_data.sched_cache)
            mlsl_sched_cache_free(global_data.sched_cache);

        global_data.ooo_handler.reset();
        global_data.executor.reset();
        global_data.fusion_manager.reset();
        global_data.comm.reset();
        global_data.comm_ids.reset();
        global_data.parallelizer.reset();

        if (global_data.default_coll_attr)
            mlsl_coll_free_attr(global_data.default_coll_attr);

        mlsl_env_free();

#ifdef ENABLE_DEBUG
        //simple memory leak checking, does not guarantee strong precision since global objects may be destroyed after
        //call of mlsl_finalize and may impact on memory allocation

        size_t allocations_cnt = allocations_count.load();
        size_t deallocations_cnt = deallocations_count.load();
        MLSL_LOG(INFO, "Operator new called %zu times, operator free called %zu times", allocations_cnt, deallocations_cnt);
        if(allocations_cnt != deallocations_cnt)
        {
            //todo: use MLSL_LOG(ERROR, ...) which does not throw
            fprintf(stderr, "Alloc (%zu) / dealloc (%zu) mismatch, possible memory leak\n", allocations_cnt, deallocations_cnt);
        }
#endif

        return mlsl_status_success;
    }
    //todo: MLSL_LOG(ERROR) calls mlsl_finalize which leads to recursive calls
    // MLSL_LOG(ERROR) needs to be reworked
    catch(...)
    {
        fprintf(stderr, "general error in mlsl_finalize()\n");
        return mlsl_status_runtime_error;
    }
    //COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_wait(mlsl_request *req)
{
    try
    {
        if (!req)
        {
            MLSL_ASSERTP(0);
            return mlsl_status_success;
        }

        global_data.executor->wait(req);
        mlsl_sched* sched = req->sched;
        MLSL_ASSERT(sched);

        if (!sched->coll_attr.to_cache)
            delete sched;

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_test(mlsl_request *req, int *is_completed)
{
    try
    {
        if (!req)
        {
            MLSL_ASSERTP(0);
            if (is_completed) *is_completed = 1;
            return mlsl_status_success;
        }

        bool completed = global_data.executor->test(req);

        if (completed)
        {
            MLSL_LOG(DEBUG, "test of req %p completed", req);
            mlsl_sched* sched = req->sched;
            MLSL_ASSERTP(sched);

            if (!sched->coll_attr.to_cache)
                delete sched;
        }

        *is_completed = static_cast<int>(completed);

        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_comm_create(mlsl_comm_t* comm_t,
                               mlsl_comm_attr_t* comm_attr)
{
    MLSL_ASSERT(comm_t);
    try
    {
        mlsl_comm* comm = nullptr;
        if (!comm_attr)
        {
            MLSL_LOG(DEBUG, "Duplicating global comm");
            comm = new mlsl_comm(global_data.comm->rank(), global_data.comm->size(), std::unique_ptr<comm_id>(new comm_id(*global_data.comm_ids)));
        }
        else
        {

            comm = mlsl_comm::create_with_color(comm_attr->color, global_data.comm_ids.get(), global_data.comm.get());
        }

        *comm_t = static_cast<void*>(comm);
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t mlsl_comm_free(mlsl_comm_t comm_t)
{
    MLSL_ASSERT(comm_t);
    MLSL_LOG(DEBUG, "Free comm %p", comm_t);
    try
    {
        mlsl_comm* comm = static_cast<mlsl_comm*>(comm_t);
        delete comm;
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_get_comm_rank(mlsl_comm_t comm_t,
                                          size_t* out_rank)
{
    try
    {
        mlsl_comm* comm = static_cast<mlsl_comm*>(comm_t);
        auto rank = comm ? comm->rank() : global_data.comm->rank();
        if (out_rank)
        {
            *out_rank = rank;
        }
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_get_comm_size(mlsl_comm_t comm_t,
                                          size_t* out_size)
{
    try
    {
        mlsl_comm* comm = static_cast<mlsl_comm*>(comm_t);
        auto size = comm ? comm->size() : global_data.comm->size();
        if (out_size)
        {
            *out_size = size;
        }
        return mlsl_status_success;
    }
    COMMON_CATCH_BLOCK()
}

mlsl_status_t MLSL_API mlsl_get_max_comm_count(size_t* out_count)
{
    if(out_count != nullptr)
    {
        *out_count = mlsl_comm::max_comm_count;
    }

    return mlsl_status_success;
}

mlsl_status_t MLSL_API mlsl_get_current_comm_count(size_t* out_count)
{
    if(out_count != nullptr)
    {
        *out_count = mlsl_comm::comm_count.load();
    }

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
