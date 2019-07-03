#include "fusion/fusion.hpp"
#include "sched/entry_factory.hpp"
#include "sched/sched_cache.hpp"

#define ICCL_FUSION_CHECK_SCHEDS_ITERS (1024)

iccl_status_t complete_user_request(const void* ctx)
{
    iccl_sched* sched = (iccl_sched*) ctx;
    LOG_DEBUG("completing request");
    sched->req->complete();
    return iccl_status_success;
}

iccl_status_t release_fusion_buf(const void* ctx)
{
    void* buf = (void*) ctx;
    global_data.fusion_manager->release_buffer(buf);
    return iccl_status_success;
}

iccl_status_t release_fusion_buf_for_cached_sched(iccl_sched* sched,
                                                  const void* ctx)
{
    return release_fusion_buf(ctx);
}

iccl_buffer_cache::iccl_buffer_cache(size_t buf_size)
    : buf_size(buf_size)
{
    void* buf;
    for (size_t idx = 0; idx < ICCL_BUFFER_CACHE_PREALLOC; idx++)
    {
        buf = ICCL_MALLOC(buf_size, "buffer");
        free_buffers.push_back(buf);
        all_buffers.push_back(buf);
    }
    LOG_INFO("created buffer_cache: buf_size ", buf_size);
}

iccl_buffer_cache::~iccl_buffer_cache()
{
    std::lock_guard<fusion_lock_t> lock{guard};
    if(free_buffers.size() != all_buffers.size())
    {
        ICCL_FATAL("size mismatch ", free_buffers.size(), " vs ", all_buffers.size());
    }

    for (size_t idx = 0; idx < all_buffers.size(); idx++)
    {
        ICCL_FREE(all_buffers[idx]);
    }
    all_buffers.clear();
    free_buffers.clear();
}

void* iccl_buffer_cache::get()
{
    std::lock_guard<fusion_lock_t> lock{guard};
    void* buf;
    if (!free_buffers.empty())
    {
        buf = free_buffers.front();
        free_buffers.pop_front();
    }
    else
    {
        buf = ICCL_MALLOC(buf_size, "buffer");
        LOG_DEBUG("get buf from extra allocation ", buf);
        all_buffers.push_back(buf);
    }
    ICCL_THROW_IF_NOT(buf, "empty buf");
    return buf;
}

void iccl_buffer_cache::release(void* buf)
{
    std::lock_guard<fusion_lock_t> lock{guard};
    ICCL_THROW_IF_NOT(buf, "empty buf");
    free_buffers.push_back(buf);
}

iccl_fusion_manager::iccl_fusion_manager()
    : bytes_threshold(env_data.fusion_bytes_threshold),
      count_threshold(env_data.fusion_count_threshold),
      buf_cache(env_data.fusion_bytes_threshold * env_data.fusion_count_threshold)
{
    ICCL_ASSERT(bytes_threshold >= 1, "unexpected fusion_bytes_threshold ",
                    bytes_threshold);
    ICCL_ASSERT(count_threshold >= 1, "unexpected fusion_count_threshold ",
                    count_threshold);

    long cycle_usec = long(env_data.fusion_cycle_ms * 1000.0);
    cycle = std::chrono::microseconds(cycle_usec);
    last_exec_time = std::chrono::steady_clock::now();

    LOG_INFO("created fusion_manager manager, cycle_usec ", cycle_usec, ", bytes_threshold ", bytes_threshold,
        ", count_threshold ", count_threshold);
}

iccl_fusion_manager::~iccl_fusion_manager()
{
    LOG_INFO("fused_bytes ", stat_fused_bytes, ", fused_ops ", stat_fused_ops,
        ", empty_exec_calls ", stat_empty_exec_calls);

    check_tracked_scheds();

    ICCL_ASSERT(postponed_queue.empty() && exec_queue.empty() && tracked_scheds.empty(),
                    "queues are not empty, ", postponed_queue.size(), " ",
                    exec_queue.size(), " ", tracked_scheds.size());
}

bool iccl_fusion_manager::can_fuse(iccl_sched* sched)
{
    size_t bytes = sched->coll_param.count * iccl_datatype_get_size(sched->coll_param.dtype);
    if (bytes >= bytes_threshold)
    {
        LOG_DEBUG("can't fuse due to size ", bytes , ", max ", bytes_threshold);
        return false;
    }

    if (sched->coll_param.ctype != iccl_coll_allreduce)
    {
        LOG_DEBUG("can't fuse due to coll_type ", iccl_coll_type_to_str(sched->coll_param.ctype));
        return false;
    }

    if (sched->coll_attr.prologue_fn ||
        sched->coll_attr.epilogue_fn ||
        sched->coll_attr.reduction_fn ||
        sched->coll_attr.synchronous ||
        sched->coll_attr.match_id.length())
    {
        LOG_DEBUG("can't fuse due to unexpected fields in coll_attr");
        return false;
    }

    LOG_DEBUG("can fuse, bytes ", bytes);
    return true;
}

bool iccl_fusion_manager::add(iccl_sched* sched)
{
    if (!can_fuse(sched))
    { return false; }

    ICCL_THROW_IF_NOT(!env_data.fusion_check_urgent || !sched->urgent, "incorrect values : ",
                      env_data.fusion_check_urgent, " vs ", sched->urgent);
    ICCL_THROW_IF_NOT(sched->req->is_completed(), "incorrect completion counter");
    sched->req->set_counter(1);

    std::lock_guard<fusion_lock_t> lock{guard};
    postponed_queue.push_back(sched);
    return true;
}

iccl_sched* iccl_fusion_manager::build_sched()
{
    size_t sum_count = 0, sum_bytes = 0, dtype_size;
    size_t max_priority = 0;
    bool use_cache = true;
    iccl_comm* comm;
    iccl_datatype_internal_t dtype;
    iccl_reduction_t reduction;
    iccl_coll_type ctype;
    void* fusion_buf = nullptr;

    ICCL_THROW_IF_NOT(exec_queue.size(), "empty queue");

    auto first_sched = exec_queue.front();
    auto last_sched = exec_queue.back();
    dtype = first_sched->coll_param.dtype;
    dtype_size = iccl_datatype_get_size(dtype);
    reduction = first_sched->coll_param.reduction;
    comm = first_sched->coll_param.comm;
    ctype = first_sched->coll_param.ctype;
    max_priority = first_sched->coll_attr.priority;

    for (auto it = exec_queue.begin(); it != exec_queue.end(); ++it)
    {
        auto s = *it;
        sum_count += s->coll_param.count;
        if (!s->coll_attr.to_cache)
        {
            use_cache = false;
        }
        if (s->coll_attr.priority > max_priority)
        {
            max_priority = s->coll_attr.priority;
        }
    }
    sum_bytes = sum_count * dtype_size;

    LOG_DEBUG("build fused_sched for sum_count ", sum_count, ", sum_bytes ", sum_bytes,
        ", sched_count ", exec_queue.size());

    iccl_sched* sched = nullptr;
    iccl_sched_key key{};
    if (use_cache)
    {
        key.ctype = iccl_coll_allreduce;
        key.buf1 = (void*) first_sched;
        key.buf2 = (void*) first_sched->coll_param.send_buf;
        key.buf3 = (void*) last_sched->coll_param.send_buf;
        key.count1 = sum_count;
        key.count2 = exec_queue.size();
        key.dtype = dtype->type;
        key.reduction = reduction;
        key.comm = comm;
        sched = global_data.sched_cache->find(key);
    }

    stat_fused_bytes += sum_bytes;
    stat_fused_ops += exec_queue.size();

    if (!sched)
    {
        iccl_coll_param coll_param{};
        LOG_DEBUG("didn't find allreduce fused_sched in cache");
        switch (ctype)
        {
            case iccl_coll_allreduce:
                fusion_buf = buf_cache.get();
                coll_param.ctype = iccl_coll_allreduce;
                coll_param.send_buf = fusion_buf;
                coll_param.recv_buf = fusion_buf;
                coll_param.count = sum_count;
                coll_param.dtype = dtype;
                coll_param.reduction = reduction;
                coll_param.comm = comm;
                sched = new iccl_sched(coll_param);
                sched->internal_type = iccl_sched_internal_fusion;
                sched->coll_attr.priority = max_priority;
                break;
            default:
                ICCL_FATAL("not supported");
                break;
        }
        sched->commit(global_data.parallelizer.get());
        sched->coll_attr.to_cache = (use_cache) ? 1 : 0;

        if (use_cache)
        {
            global_data.sched_cache->add(key, sched);
        }
    }
    else
    {
        LOG_DEBUG("found allreduce fused_sched in cache");
        ICCL_THROW_IF_NOT(sched->req->is_completed(), "non completed sched found in cache");
        clear_exec_queue();
        return sched;
    }

    size_t exec_queue_size = exec_queue.size();
    size_t part_count = sched->partial_scheds.size();
    std::vector<std::shared_ptr<iccl_sched>>& part_scheds = sched->partial_scheds;
    size_t copies_per_part = exec_queue_size / part_count;
    size_t copies_per_last_part = copies_per_part + exec_queue_size % part_count;
    std::shared_ptr<sched_entry> e;

    ICCL_ASSERT(part_count > 0);
    LOG_DEBUG("part_count ", part_count,
        ", sum_count ", sum_count,
        ", exec_queue_size ", exec_queue_size);

    for (size_t idx = 0; idx < part_count; idx++)
    {
        part_scheds[idx]->add_barrier();
        part_scheds[idx]->set_add_mode(iccl_sched_add_front);
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
                                           (char*) fusion_buf + offset,
                                           exec_queue[global_copy_idx]->coll_param.count,
                                           dtype);
            offset += exec_queue[global_copy_idx]->coll_param.count * dtype_size;
        }
    }

    for (size_t idx = 0; idx < part_count; idx++)
    {
        part_scheds[idx]->set_add_mode(iccl_sched_add_back);
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
                                           (char*) fusion_buf + offset,
                                           exec_queue[global_copy_idx]->coll_param.recv_buf,
                                           exec_queue[global_copy_idx]->coll_param.count,
                                           dtype);
            offset += exec_queue[global_copy_idx]->coll_param.count * dtype_size;
            entry_factory::make_function_entry(part_scheds[idx].get(),
                                               complete_user_request,
                                               exec_queue[global_copy_idx]);
            ICCL_THROW_IF_NOT(!exec_queue[global_copy_idx]->req->is_completed(),
                              "incorrect completion counter");
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
    {
        tracked_scheds.push_back(sched);
    }

    clear_exec_queue();

    return sched;
}

void iccl_fusion_manager::execute()
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
                LOG_DEBUG("found urgent sched in exec_queue, flush_exec_queue");
                flush_exec_queue = true;
                break;
            }
        }
    }

    /* separate block to reduce lock scope */
    {
        std::lock_guard<fusion_lock_t> lock{guard};
        if (!postponed_queue.empty())
        {
            LOG_DEBUG("postponed_queue size ", postponed_queue.size());

            iccl_sched* first_sched;
            if (!exec_queue.empty())
            {
                first_sched = exec_queue.front();
            }
            else
            {
                first_sched = postponed_queue.front();
                exec_queue.push_back(first_sched);
                postponed_queue.pop_front();
                exec_queue_sum_bytes = first_sched->coll_param.count *
                                       iccl_datatype_get_size(first_sched->coll_param.dtype);
            }

            for (auto it = postponed_queue.begin(); it != postponed_queue.end();)
            {
                auto s = *it;
                if (s->coll_param.dtype == first_sched->coll_param.dtype &&
                    s->coll_param.comm == first_sched->coll_param.comm &&
                    s->coll_param.ctype == first_sched->coll_param.ctype &&
                    s->coll_param.reduction == first_sched->coll_param.reduction)
                {
                    size_t size = s->coll_param.count * iccl_datatype_get_size(s->coll_param.dtype);
                    if (exec_queue_sum_bytes + size > ICCL_FUSION_BUFFER_SIZE)
                    {
                        LOG_DEBUG("too much bytes in buffer, flush_exec_queue");
                        flush_exec_queue = true;
                        break;
                    }
                    exec_queue_sum_bytes += size;

                    if (env_data.fusion_check_urgent && !flush_exec_queue && s->urgent)
                    {
                        LOG_DEBUG("found urgent sched in postponed_queue, flush_exec_queue, postponed_queue size ",
                                 postponed_queue.size());
                        flush_exec_queue = true;
                    }

                    exec_queue.push_back(s);
                    it = postponed_queue.erase(it);

                    if (exec_queue.size() == count_threshold)
                    {
                        LOG_DEBUG("too many scheds, flush_exec_queue");
                        flush_exec_queue = true;
                        break;
                    }
                }
                else
                {
                    ++it;
                }
            }
        }
    }

    if (flush_exec_queue)
    {
        LOG_DEBUG("exec_queue size ", exec_queue.size(), ", bytes ", exec_queue_sum_bytes);
        iccl_sched* sched = build_sched();
        sched->start(global_data.executor.get());
    }

    if (stat_fused_ops % ICCL_FUSION_CHECK_SCHEDS_ITERS == 0)
    {
        check_tracked_scheds();
    }
}

void iccl_fusion_manager::release_buffer(void* buf)
{
    buf_cache.release(buf);
}

void iccl_fusion_manager::clear_exec_queue()
{
    exec_queue.clear();
    exec_queue_sum_bytes = 0;
}

void iccl_fusion_manager::check_tracked_scheds()
{
    for (auto it = tracked_scheds.begin(); it != tracked_scheds.end();)
    {
        iccl_sched* sched = *it;
        if (sched->req->is_completed())
        {
            delete sched;
            it = tracked_scheds.erase(it);
        }
        else
        {
            ++it;
        }
    }
}
