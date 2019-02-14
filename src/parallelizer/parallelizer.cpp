#include "parallelizer/parallelizer.hpp"
#include "sched/entry_factory.hpp"
#include "common/datatype/datatype.hpp"
#include "comp/comp.hpp"

#define MLSL_MIN_PART_SIZE (2048)

static size_t lifo_priority = 0;

mlsl_parallelizer* global_parallelizer = NULL;

typedef struct
{
    /* keep these 4 fields on the top of structure */
    void* buf;
    size_t count;
    mlsl_datatype_t dtype;
    size_t dtype_size;
    /*---*/

    mlsl_datatype_internal dtype_internal;
    size_t part_idx;
    size_t part_count;
}
    mlsl_parallelizer_prologue_ctx;

mlsl_status_t mlsl_parallelizer_prologue_get_buf(const void* ctx, void* field_ptr)
{
    mlsl_parallelizer_prologue_ctx* pctx = (mlsl_parallelizer_prologue_ctx*)ctx;
    void** buf_ptr = (void**)field_ptr;
    *buf_ptr = (char*)pctx->buf + pctx->part_idx * (pctx->count / pctx->part_count) * pctx->dtype_size;
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_prologue_get_count(const void* ctx, void* field_ptr)
{
    mlsl_parallelizer_prologue_ctx* pctx = (mlsl_parallelizer_prologue_ctx*)ctx;
    size_t count = pctx->count / pctx->part_count;
    if (pctx->part_idx == (pctx->part_count - 1)) count += pctx->count % pctx->part_count;
    size_t* count_ptr = (size_t*)field_ptr;
    *count_ptr = count;
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_prologue_get_dtype(const void* ctx, void* field_ptr)
{
    mlsl_parallelizer_prologue_ctx* pctx = (mlsl_parallelizer_prologue_ctx*)ctx;
    mlsl_datatype_internal_t* dtype_ptr = (mlsl_datatype_internal_t*)field_ptr;
    pctx->dtype_internal.type = pctx->dtype;
    pctx->dtype_internal.size = pctx->dtype_size;
    pctx->dtype_internal.name = "UNKNOWN";
    *dtype_ptr = &pctx->dtype_internal;
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_create(size_t partition_count,
                                       mlsl_parallelizer** parallelizer)
{
    mlsl_parallelizer* p = static_cast<mlsl_parallelizer*>(MLSL_CALLOC(sizeof(mlsl_parallelizer), "parallelizer"));
    p->partition_count = partition_count;
    *parallelizer = p;
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_free(mlsl_parallelizer* parallelizer)
{
    MLSL_FREE(parallelizer);
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_process(mlsl_parallelizer* parallelizer,
                                        mlsl_sched* sched,
                                        mlsl_sched*** scheds,
                                        size_t* sched_count)
{
    MLSL_ASSERT(sched);

    mlsl_status_t status = mlsl_status_success;
    size_t part_count = 1, idx, base_count, dtype_size;
    mlsl_sched_coll_param* coll_param = &(sched->coll_param);
    mlsl_datatype_internal_t dtype = coll_param->dtype;
    dtype_size = mlsl_datatype_get_size(dtype);
    mlsl_coll_type coll_type = coll_param->ctype;

    size_t* counts = NULL, * offsets = NULL;
    mlsl_sched** part_scheds = NULL;
    size_t* recv_counts = NULL;
    std::shared_ptr<sched_entry> e;

    switch (coll_type)
    {
        case mlsl_coll_barrier:
            part_count = 1;
            break;
        case mlsl_coll_bcast:
        case mlsl_coll_reduce:
        case mlsl_coll_allreduce:
        case mlsl_coll_custom:
            if (coll_param->count * dtype_size <= MLSL_MIN_PART_SIZE)
            {
                part_count = 1;
            }
            else
            {
                part_count = parallelizer->partition_count;
            }
            break;
        case mlsl_coll_allgatherv:
            part_count = coll_param->comm->size();
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, coll_type %d, part_count %zu",
             sched, sched->entries.size(), coll_type, part_count);

    part_scheds = static_cast<mlsl_sched**>(MLSL_MALLOC(sizeof(mlsl_sched*) * part_count, "scheds"));
    if (coll_type == mlsl_coll_custom)
    {
        MLSL_ASSERTP(0);
    }
    else
    {
        counts = static_cast<size_t*>(MLSL_MALLOC(sizeof(size_t) * part_count, "counts"));
        offsets = static_cast<size_t*>(MLSL_MALLOC(sizeof(size_t) * part_count, "counts"));
        for (idx = 0; idx < part_count; idx++)
        {
            part_scheds[idx] = new mlsl_sched{};
            part_scheds[idx]->coll_param.comm = sched->coll_param.comm;
            part_scheds[idx]->coll_param.ctype = sched->coll_param.ctype;
            part_scheds[idx]->root = sched;
        }
    }

    for (idx = 0; idx < part_count; idx++)
    {
        memcpy(&(part_scheds[idx]->coll_attr), &(sched->coll_attr), sizeof(mlsl_sched_coll_attr));
        if (env_data.priority_mode == mlsl_priority_lifo)
        {
            part_scheds[idx]->coll_attr.priority = lifo_priority;
        }
    }

    if (env_data.priority_mode == mlsl_priority_lifo)
    {
        lifo_priority++;
    }

    switch (coll_type)
    {
        case mlsl_coll_barrier:
            break;
        case mlsl_coll_bcast:
        case mlsl_coll_reduce:
        case mlsl_coll_allreduce:
            base_count = coll_param->count / part_count;
            for (idx = 0; idx < part_count; idx++)
            {
                counts[idx] = base_count;
                offsets[idx] = idx * counts[idx] * dtype_size;
            }
            counts[part_count - 1] += coll_param->count % part_count;
            break;
        case mlsl_coll_allgatherv:
            recv_counts = coll_param->recv_counts;
            MLSL_ASSERTP(recv_counts);
            counts[0] = recv_counts[0];
            offsets[0] = 0;
            for (idx = 1; idx < part_count; idx++)
            {
                counts[idx] = recv_counts[idx];
                offsets[idx] = offsets[idx - 1] + counts[idx - 1] * dtype_size;
            }
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    switch (coll_type)
    {
        case mlsl_coll_barrier:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_barrier(part_scheds[idx]));
            }
            break;
        case mlsl_coll_bcast:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_bcast(part_scheds[idx],
                                                (char*) coll_param->buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                coll_param->root));
            }
            break;
        case mlsl_coll_reduce:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_reduce(part_scheds[idx],
                                                 (char*) coll_param->send_buf + offsets[idx],
                                                 (char*) coll_param->recv_buf + offsets[idx],
                                                 counts[idx],
                                                 dtype,
                                                 coll_param->reduction,
                                                 coll_param->root));
            }
            break;
        case mlsl_coll_allreduce:
            mlsl_parallelizer_prologue_ctx* main_ctx;
            if (sched->coll_attr.prologue_fn)
            {
                MLSL_CALL(mlsl_sched_alloc_buffer(part_scheds[0], sizeof(mlsl_parallelizer_prologue_ctx), (void**)&main_ctx));
                main_ctx->part_idx = 0;
                main_ctx->part_count = 1;
                entry_factory::make_prologue_entry(part_scheds[0],
                                                   sched->coll_attr.prologue_fn,
                                                   (char*) coll_param->send_buf,
                                                   coll_param->count,
                                                   dtype,
                                                   &(main_ctx->buf),
                                                   &(main_ctx->count),
                                                   &(main_ctx->dtype),
                                                   &(main_ctx->dtype_size));
                MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));
                for (idx = 0; idx < part_count; idx++)
                {
                    mlsl_parallelizer_prologue_ctx* part_ctx;
                    MLSL_CALL(mlsl_sched_alloc_buffer(part_scheds[idx], sizeof(mlsl_parallelizer_prologue_ctx),
                                                      (void**)&part_ctx));
                    part_ctx->part_idx = idx;
                    part_ctx->part_count = part_count;
                    entry_factory::make_copy_entry(part_scheds[idx],
                                                   main_ctx,
                                                   part_ctx,
                                                   sizeof(void*) + sizeof(size_t) +
                                                   sizeof(mlsl_datatype_t) + sizeof(size_t),
                                                   mlsl_dtype_internal_char);
                    e = entry_factory::make_coll_entry(part_scheds[idx],
                                                       mlsl_coll_allreduce,
                                                       NULL, /* send_buf */
                                                       NULL, /* recv_buf */
                                                       0, /* count */
                                                       mlsl_dtype_internal_none,
                                                       coll_param->reduction);
                    e->set_field_fn(mlsl_sched_entry_field_send_buf,
                                    mlsl_parallelizer_prologue_get_buf, part_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_recv_buf,
                                    mlsl_parallelizer_prologue_get_buf, part_ctx, false); // in-place
                    e->set_field_fn(mlsl_sched_entry_field_cnt,
                                    mlsl_parallelizer_prologue_get_count, part_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_dtype,
                                    mlsl_parallelizer_prologue_get_dtype, part_ctx, false);
                }
                if (!sched->coll_attr.epilogue_fn)
                {
                    MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));
                    e = entry_factory::make_copy_entry(part_scheds[0],
                                                       NULL, /* in_buf */
                                                       coll_param->recv_buf,
                                                       0, /* count */
                                                       mlsl_dtype_internal_none);
                    e->set_field_fn(mlsl_sched_entry_field_in_buf,
                                    mlsl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_cnt,
                                    mlsl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_dtype,
                                    mlsl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }
            else
            {
                for (idx = 0; idx < part_count; idx++)
                {
                    MLSL_CALL(mlsl_coll_build_allreduce(part_scheds[idx],
                                                        (char*) coll_param->send_buf + offsets[idx],
                                                        (char*) coll_param->recv_buf + offsets[idx],
                                                        counts[idx],
                                                        dtype,
                                                        coll_param->reduction));
                }
            }
            if (sched->coll_attr.epilogue_fn)
            {
                MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));
                e = entry_factory::make_epilogue_entry(part_scheds[0],
                                                       sched->coll_attr.epilogue_fn,
                                                       (char*)coll_param->recv_buf,
                                                       coll_param->count,
                                                       dtype,
                                                       (char*)coll_param->recv_buf,
                                                       coll_param->count,
                                                       dtype);
                if (sched->coll_attr.prologue_fn)
                {
                    e->set_field_fn(mlsl_sched_entry_field_in_buf,
                                    mlsl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_in_cnt,
                                    mlsl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(mlsl_sched_entry_field_in_dtype,
                                    mlsl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }
            break;
        case mlsl_coll_allgatherv:
            if (coll_param->send_buf != coll_param->recv_buf)
            {
                size_t* copy_counts = static_cast<size_t*>(MLSL_MALLOC(part_count * sizeof(size_t), "copy_counts"));
                size_t* copy_offsets = static_cast<size_t*>(MLSL_MALLOC(part_count * sizeof(size_t), "copy_offsets"));
                for (idx = 0; idx < part_count; idx++)
                {
                    copy_counts[idx] = counts[coll_param->comm->rank()] / part_count;
                    copy_offsets[idx] = idx * copy_counts[idx] * dtype_size;
                }
                copy_counts[part_count - 1] += counts[coll_param->comm->rank()] % part_count;
                for (idx = 0; idx < part_count; idx++)
                {
                    entry_factory::make_copy_entry(part_scheds[idx],
                                                   (char*) coll_param->send_buf +
                                                   copy_offsets[idx],
                                                   (char*) coll_param->recv_buf +
                                                   offsets[coll_param->comm->rank()] +
                                                   copy_offsets[idx],
                                                   copy_counts[idx], dtype);
                }
                mlsl_sched_sync_schedules(part_scheds, part_count);
                MLSL_FREE(copy_counts);
                MLSL_FREE(copy_offsets);
            }

            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_bcast(part_scheds[idx],
                                                (char*) coll_param->recv_buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                idx));
            }
            break;
        case mlsl_coll_custom:
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    MLSL_FREE(counts);
    MLSL_FREE(offsets);

    *scheds = part_scheds;
    *sched_count = part_count;

    return status;
}