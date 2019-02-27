#include "common/global/global.hpp"
#include "fusion/fusion.hpp"
#include "sched/entry_factory.hpp"

#include <algorithm>
#include <type_traits>

#define MLSL_FUSION_CHECK_SCHEDS_ITERS (1024)

mlsl_status_t complete_user_request(const void* ctx)
{
    mlsl_sched* sched = (mlsl_sched*)ctx;
    MLSL_ASSERTP(sched->req->completion_counter == 1);
    mlsl_request_complete(sched->req);
    return mlsl_status_success;
}

mlsl_status_t release_fusion_buf(const void* ctx)
{
    void* buf = (void*)ctx;
    global_data.fusion_manager->release_buffer(buf);
    return mlsl_status_success;
}

mlsl_status_t release_fusion_buf_for_cached_sched(mlsl_sched* sched, const void* ctx)
{
    return release_fusion_buf(ctx);
}

mlsl_buffer_cache::mlsl_buffer_cache(size_t buf_size)
    : buf_size(buf_size)
{
    void* buf;
    for (size_t idx = 0; idx < MLSL_BUFFER_CACHE_PREALLOC; idx++)
    {
        buf = MLSL_MALLOC(buf_size, "buffer");
        free_buffers.push_back(buf);
        all_buffers.push_back(buf);
    }
    MLSL_LOG(INFO, "created buffer_cache: buf_size %zu", buf_size);
}

mlsl_buffer_cache::~mlsl_buffer_cache()
{
    std::lock_guard<std::mutex> lock{guard};
    MLSL_ASSERTP(free_buffers.size() == all_buffers.size());
    for (size_t idx = 0; idx < all_buffers.size(); idx++)
    {
        MLSL_FREE(all_buffers[idx]);
    }
    all_buffers.clear();
    free_buffers.clear();
}

void* mlsl_buffer_cache::get()
{
    std::lock_guard<std::mutex> lock{guard};
    void* buf;
    if (!free_buffers.empty())
    {
        buf = free_buffers.front();
        free_buffers.pop_front();
    }
    else
    {
        buf = MLSL_MALLOC(buf_size, "buffer");
        MLSL_LOG(DEBUG, "get buf from extra allocation %p", buf);
        all_buffers.push_back(buf);
    }
    MLSL_ASSERTP(buf);
    return buf;
}

void mlsl_buffer_cache::release(void* buf)
{
    std::lock_guard<std::mutex> lock{guard};
    MLSL_ASSERTP(buf);
    free_buffers.push_back(buf);
}

mlsl_fusion_manager::mlsl_fusion_manager()
    : bytes_threshold(env_data.fusion_bytes_threshold),
      count_threshold(env_data.fusion_count_threshold),
      buf_cache(env_data.fusion_bytes_threshold * env_data.fusion_count_threshold)
{
    MLSL_ASSERTP_FMT(bytes_threshold >= 1, "unexpected fusion_bytes_threshold %zu",
        bytes_threshold);
    MLSL_ASSERTP_FMT(count_threshold >= 1, "unexpected fusion_count_threshold %zu",
        count_threshold);

    long cycle_usec = long(env_data.fusion_cycle_ms * 1000.0);
    cycle = std::chrono::microseconds(cycle_usec);
    last_exec_time = std::chrono::steady_clock::now();

    MLSL_LOG(INFO, "created fusion manager, cycle_usec %ld, bytes_threshold %zu, count_threshold %zu",
        cycle_usec, bytes_threshold, count_threshold);
}

mlsl_fusion_manager::~mlsl_fusion_manager()
{
    MLSL_LOG(INFO, "fused_bytes %zu, fused_ops %zu, empty_exec_calls %zu",
             stat_fused_bytes, stat_fused_ops, stat_empty_exec_calls);

    check_tracked_scheds();

    MLSL_ASSERTP(postponed_queue.empty() &&
                 exec_queue.empty() &&
                 tracked_scheds.empty());
}

bool mlsl_fusion_manager::can_fuse(mlsl_sched* sched)
{
    size_t bytes = sched->coll_param.count * mlsl_datatype_get_size(sched->coll_param.dtype);
    if (bytes >= bytes_threshold)
    {
        MLSL_LOG(DEBUG, "can't fuse due to size %zu, max %zu", bytes,
                 bytes_threshold);
        return false;
    }

    if (sched->coll_param.ctype != mlsl_coll_allreduce)
    {
        MLSL_LOG(DEBUG, "can't fuse due to coll_type %s", mlsl_coll_type_to_str(sched->coll_param.ctype));
        return false;
    }

    if (sched->coll_attr.prologue_fn  ||
        sched->coll_attr.epilogue_fn  ||
        sched->coll_attr.reduction_fn ||
        sched->coll_attr.synchronous  ||
        strlen(sched->coll_attr.match_id))
    {
        MLSL_LOG(DEBUG, "can't fuse due to unexpected fields in coll_attr");
        return false;
    }

    MLSL_LOG(DEBUG, "can fuse, bytes %zu", bytes);
    return true;
}

bool mlsl_fusion_manager::add(mlsl_sched* sched)
{
    if (!can_fuse(sched)) return false;

    MLSL_ASSERTP(!env_data.fusion_check_urgent || sched->urgent == false);
    MLSL_ASSERTP(sched->req->completion_counter == 0);
    sched->req->completion_counter = 1;

    std::lock_guard<std::mutex> lock{guard};
    postponed_queue.push_back(sched);
    return true;
}

mlsl_sched* mlsl_fusion_manager::build_sched()
{
    size_t sum_count = 0, sum_bytes = 0, dtype_size;
    size_t max_priority = 0;
    bool use_cache = true;
    mlsl_comm* comm;
    mlsl_datatype_internal_t dtype;
    mlsl_reduction_t reduction;
    mlsl_coll_type ctype;
    void* fusion_buf = nullptr;

    MLSL_ASSERTP(exec_queue.size());

    auto first_sched = exec_queue.front();
    auto last_sched = exec_queue.back();
    dtype = first_sched->coll_param.dtype;
    dtype_size = mlsl_datatype_get_size(dtype);
    reduction = first_sched->coll_param.reduction;
    comm = first_sched->coll_param.comm;
    ctype = first_sched->coll_param.ctype;
    max_priority = first_sched->coll_attr.priority;

    for (auto it = exec_queue.begin(); it != exec_queue.end(); ++it)
    {
        auto s = *it;
        sum_count += s->coll_param.count;
        if (!s->coll_attr.to_cache)
            use_cache = false;
        if (s->coll_attr.priority > max_priority)
            max_priority = s->coll_attr.priority;
    }
    sum_bytes = sum_count * dtype_size;

    MLSL_LOG(DEBUG, "build fused_sched for sum_count %zu, sum_bytes %zu, sched_count %zu",
             sum_count, sum_bytes, exec_queue.size());

    mlsl_sched* sched = nullptr;
    mlsl_sched_cache_entry* entry = nullptr;

    if (use_cache)
    {
        mlsl_sched_cache_key key{};
        key.ctype = mlsl_coll_allreduce;
        key.buf1 = (void*)first_sched;
        key.buf2 = (void*)first_sched->coll_param.send_buf;
        key.buf3 = (void*)last_sched->coll_param.send_buf;
        key.count1 = sum_count;
        key.count2 = exec_queue.size();
        key.dtype = dtype->type;
        key.reduction = reduction;
        key.comm = comm;
        mlsl_sched_cache_get_entry(global_data.sched_cache, &key, &entry);
        sched = entry->sched;
    }

    stat_fused_bytes += sum_bytes;
    stat_fused_ops += exec_queue.size();

    if (!sched)
    {
        mlsl_coll_param coll_param{};
        MLSL_LOG(DEBUG, "didn't find allreduce fused_sched in cache");
        switch (ctype)
        {
            case mlsl_coll_allreduce:
                fusion_buf = buf_cache.get();
                coll_param.ctype = mlsl_coll_allreduce;
                coll_param.send_buf = fusion_buf;
                coll_param.recv_buf = fusion_buf;
                coll_param.count = sum_count;
                coll_param.dtype = dtype;
                coll_param.reduction = reduction;
                coll_param.comm = comm;
                sched = new mlsl_sched(coll_param);
                sched->is_internal = true;
                sched->coll_attr.priority = max_priority;
                break;
            default:
                MLSL_ASSERTP(0);
                break;
        }
        sched->commit(global_data.parallelizer.get());
        sched->coll_attr.to_cache = (use_cache) ? 1 : 0;
        if (entry) entry->sched = sched;
    }
    else
    {
        MLSL_LOG(DEBUG, "found allreduce fused_sched in cache");
        MLSL_ASSERTP(mlsl_request_is_complete(sched->req));
        clear_exec_queue();
        return sched;
    }

    size_t exec_queue_size = exec_queue.size();
    size_t part_count = sched->partial_scheds.size();
    std::vector<std::shared_ptr<mlsl_sched>>& part_scheds = sched->partial_scheds;
    size_t copies_per_part = exec_queue_size / part_count;
    size_t copies_per_last_part = copies_per_part + exec_queue_size % part_count;
    std::shared_ptr<sched_entry> e;

    MLSL_ASSERTP(part_count > 0);
    MLSL_LOG(DEBUG, "part_count %zu, sum_count %zu", part_count, sum_count);

    for (size_t idx = 0; idx < part_count; idx++)
    {
        part_scheds[idx]->set_add_mode(mlsl_sched_add_front);
    }
    sched->sync_partial_scheds();

    size_t offset = 0;
    for (size_t idx = 0; idx < part_count; idx++)
    {
        size_t copies_count = (idx < part_count - 1) ?
            copies_per_part : copies_per_last_part;

        for (size_t copy_idx = 0; copy_idx < copies_count; copy_idx++)
        {
            size_t global_copy_idx = idx * copies_per_part + copy_idx;
            entry_factory::make_copy_entry(part_scheds[idx].get(),
                                           exec_queue[global_copy_idx]->coll_param.send_buf,
                                           (char*)fusion_buf + offset,
                                           exec_queue[global_copy_idx]->coll_param.count,
                                           dtype);
            offset += exec_queue[global_copy_idx]->coll_param.count * dtype_size;
        }
    }

    for (size_t idx = 0; idx < part_count; idx++)
    {
        part_scheds[idx]->set_add_mode(mlsl_sched_add_back);
    }
    sched->sync_partial_scheds();

    offset = 0;
    for (size_t idx = 0; idx < part_count; idx++)
    {
        size_t copies_count = (idx < part_count - 1) ?
            copies_per_part : copies_per_last_part;

        for (size_t copy_idx = 0; copy_idx < copies_count; copy_idx++)
        {
            size_t global_copy_idx = idx * copies_per_part + copy_idx;
            entry_factory::make_copy_entry(part_scheds[idx].get(),
                                           (char*)fusion_buf + offset,
                                           exec_queue[global_copy_idx]->coll_param.recv_buf,
                                           exec_queue[global_copy_idx]->coll_param.count,
                                           dtype);
            offset += exec_queue[global_copy_idx]->coll_param.count * dtype_size;
            entry_factory::make_function_entry(part_scheds[idx].get(),
                                               complete_user_request,
                                               exec_queue[global_copy_idx]);
            MLSL_ASSERTP(exec_queue[global_copy_idx]->req->completion_counter == 1);
        }
    }

    if (use_cache)
    {
        part_scheds[0]->set_finalize_fn(release_fusion_buf_for_cached_sched, fusion_buf);
    }
    else
    {
        sched->sync_partial_scheds();
        entry_factory::make_function_entry(part_scheds[0].get(), release_fusion_buf, fusion_buf);
    }

    if (!use_cache)
        tracked_scheds.push_back(sched);

    clear_exec_queue();
    return sched;
}

void mlsl_fusion_manager::execute()
{
    auto this_time = std::chrono::steady_clock::now();
    auto diff = (last_exec_time + cycle - this_time);
    if (diff > std::chrono::steady_clock::duration::zero())
    {
        /* it is too early, do nothing */
        stat_empty_exec_calls++;
        return;
    }
    last_exec_time = std::chrono::steady_clock::now();

    bool flush_exec_queue = false;

    if (env_data.fusion_check_urgent && !exec_queue.empty())
    {
        /* recheck scheds from exec_queue, maybe some of them were marked as urgent since previous call */
        for (auto it = exec_queue.begin(); it != exec_queue.end(); ++it)
        {
            if ((*it)->urgent)
            {
                MLSL_LOG(DEBUG, "found urgent sched in exec_queue, flush_exec_queue");
                flush_exec_queue = true;
                break;
            }
        }
    }

    /* separate block to reduce lock scope */
    {
        std::lock_guard<std::mutex> lock{guard};
        if (!postponed_queue.empty())
        {
            MLSL_LOG(DEBUG, "postponed_queue size %zu", postponed_queue.size());

            mlsl_sched* first_sched;
            if (!exec_queue.empty())
                first_sched = exec_queue.front();
            else
            {
                first_sched = postponed_queue.front();
                exec_queue.push_back(first_sched);
                postponed_queue.pop_front();
                exec_queue_sum_bytes = first_sched->coll_param.count *
                                       mlsl_datatype_get_size(first_sched->coll_param.dtype);
            }

            for (auto it = postponed_queue.begin(); it != postponed_queue.end();)
            {
                auto s = *it;
                if (s->coll_param.dtype == first_sched->coll_param.dtype &&
                    s->coll_param.comm == first_sched->coll_param.comm &&
                    s->coll_param.ctype == first_sched->coll_param.ctype &&
                    s->coll_param.reduction == first_sched->coll_param.reduction)
                {
                    size_t size = s->coll_param.count * mlsl_datatype_get_size(s->coll_param.dtype);
                    if (exec_queue_sum_bytes + size > MLSL_FUSION_BUFFER_SIZE)
                    {
                        MLSL_LOG(DEBUG, "too much bytes in buffer, flush_exec_queue");
                        flush_exec_queue = true;
                        break;
                    }
                    exec_queue_sum_bytes += size;

                    if (env_data.fusion_check_urgent && !flush_exec_queue && s->urgent)
                    {
                        MLSL_LOG(DEBUG, "found urgent sched in postponed_queue, flush_exec_queue, postponed_queue size %zu",
                                 postponed_queue.size());
                        flush_exec_queue = true;
                    }

                    exec_queue.push_back(s);
                    it = postponed_queue.erase(it);

                    if (exec_queue.size() == count_threshold)
                    {
                        MLSL_LOG(DEBUG, "too many scheds, flush_exec_queue");
                        flush_exec_queue = true;
                        break;
                    }
                }
                else
                    ++it;
            }
        }
    }

    if (flush_exec_queue)
    {
        MLSL_LOG(DEBUG, "exec_queue size %zu, bytes %zu", exec_queue.size(), exec_queue_sum_bytes);
        mlsl_sched* sched = build_sched();
        sched->start(global_data.executor.get());
    }

    if (stat_fused_ops % MLSL_FUSION_CHECK_SCHEDS_ITERS == 0)
        check_tracked_scheds();
}

void mlsl_fusion_manager::release_buffer(void* buf)
{
    buf_cache.release(buf);
}

void mlsl_fusion_manager::clear_exec_queue()
{
    exec_queue.clear();
    exec_queue_sum_bytes = 0;
}

void mlsl_fusion_manager::check_tracked_scheds()
{
    for (auto it = tracked_scheds.begin(); it != tracked_scheds.end();)
    {
        mlsl_sched* sched = *it;
        if (mlsl_request_is_complete(sched->req))
        {
            delete sched;
            it = tracked_scheds.erase(it);
        }
        else
            ++it;
    }
}
