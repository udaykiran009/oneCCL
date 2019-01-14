#include "parallelizer/parallelizer.hpp"
#include "common/datatype/datatype.hpp"
#include "comp/comp.hpp"

#define MLSL_MIN_PART_SIZE (2048)

static size_t lifo_priority = 0;

mlsl_parallelizer *global_parallelizer = NULL;

mlsl_status_t mlsl_parallelizer_create(size_t partition_count, mlsl_parallelizer **parallelizer)
{
    mlsl_parallelizer *p = static_cast<mlsl_parallelizer*>(MLSL_CALLOC(sizeof(mlsl_parallelizer), "parallelizer"));
    p->partition_count = partition_count;
    *parallelizer = p;
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_free(mlsl_parallelizer *parallelizer)
{
    MLSL_FREE(parallelizer);
    return mlsl_status_success;
}

mlsl_status_t mlsl_parallelizer_process(mlsl_parallelizer *parallelizer, mlsl_sched *sched,
                                        mlsl_sched ***scheds, size_t *sched_count)
{
    MLSL_ASSERT(sched);

    mlsl_status_t status = mlsl_status_success;
    size_t part_count = 1, idx, base_count, dtype_size;
    mlsl_sched_coll_param *coll_param = &(sched->coll_param);
    mlsl_datatype_internal_t dtype = coll_param->dtype;
    dtype_size = mlsl_datatype_get_size(dtype);
    mlsl_coll_type coll_type = coll_param->ctype;

    size_t *counts = NULL, *offsets = NULL;
    mlsl_sched **part_scheds = NULL;
    size_t *recv_counts = NULL;

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
                part_count = 1;
            else
                part_count = parallelizer->partition_count;
            break;
        case mlsl_coll_allgatherv:
            part_count = coll_param->comm->size;
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, size %zu, coll_type %d, part_count %zu",
             sched, sched->num_entries, sched->size, coll_type, part_count);

    part_scheds = static_cast<mlsl_sched**>(MLSL_MALLOC(sizeof(mlsl_sched*) * part_count, "scheds"));
    if (coll_type == mlsl_coll_custom)
    {
        MLSL_ASSERTP(0);
        for (idx = 0; idx < part_count; idx++)
            mlsl_sched_clone(sched, &part_scheds[idx]);
    }
    else
    {
        counts = static_cast<size_t*>(MLSL_MALLOC(sizeof(size_t) * part_count, "counts"));
        offsets = static_cast<size_t*>(MLSL_MALLOC(sizeof(size_t) * part_count, "counts"));
        for (idx = 0; idx < part_count; idx++)
        {
            MLSL_CALL(mlsl_sched_create(&(part_scheds[idx])));
            part_scheds[idx]->coll_param.comm = sched->coll_param.comm;
            part_scheds[idx]->coll_param.ctype = sched->coll_param.ctype;
            part_scheds[idx]->root = sched;
        }
    }

    for (idx = 0; idx < part_count; idx++)
    {
        memcpy(&(part_scheds[idx]->coll_attr), &(sched->coll_attr), sizeof(mlsl_sched_coll_attr));
        if (env_data.priority_mode == mlsl_priority_lifo)
            part_scheds[idx]->coll_attr.priority = lifo_priority;
    }

    if (env_data.priority_mode == mlsl_priority_lifo)
        lifo_priority++;

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
                                                (char *)coll_param->buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                coll_param->root));
            }
            break;
        case mlsl_coll_reduce:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_reduce(part_scheds[idx],
                                                 (char *)coll_param->send_buf + offsets[idx],
                                                 (char *)coll_param->recv_buf + offsets[idx],
                                                 counts[idx],
                                                 dtype,
                                                 coll_param->reduction,
                                                 coll_param->root));
            }
            break;
        case mlsl_coll_allreduce:
            if (sched->coll_attr.prologue_fn)
            {
                MLSL_CALL(mlsl_sched_add_prologue(part_scheds[0],
                                                  sched->coll_attr.prologue_fn,
                                                  (char *)coll_param->send_buf,
                                                  coll_param->count,
                                                  dtype,
                                                  &(sched->postponed_fields.buf),
                                                  &(sched->postponed_fields.count),
                                                  &(sched->postponed_fields.dtype)));
                MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));

                for (idx = 0; idx < part_count; idx++)
                {
                    MLSL_CALL(mlsl_sched_add_copy(part_scheds[idx],
                                                  &(sched->postponed_fields),
                                                  &(part_scheds[idx]->postponed_fields),
                                                  sizeof(mlsl_sched_postponed_fields),
                                                  mlsl_dtype_internal_char));
                    MLSL_CALL(mlsl_sched_add_update_postponed_fields(part_scheds[idx], idx, part_count));
                    MLSL_CALL(mlsl_sched_add_collective(part_scheds[idx],
                                                        mlsl_coll_allreduce,
                                                        MLSL_POSTPONED_ADDR,
                                                        MLSL_POSTPONED_ADDR,
                                                        MLSL_POSTPONED_COUNT,
                                                        MLSL_POSTPONED_DTYPE,
                                                        coll_param->reduction));
                }
                if (!sched->coll_attr.epilogue_fn)
                {
                    MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));
                    MLSL_CALL(mlsl_sched_add_copy(part_scheds[0],
                                                  &(sched->postponed_fields),
                                                  &(part_scheds[0]->postponed_fields),
                                                  sizeof(mlsl_sched_postponed_fields),
                                                  mlsl_dtype_internal_char));
                    MLSL_CALL(mlsl_sched_add_update_postponed_fields(part_scheds[0], 0, 1));
                    MLSL_CALL(mlsl_sched_add_copy(part_scheds[0],
                                                  MLSL_POSTPONED_ADDR,
                                                  coll_param->recv_buf,
                                                  MLSL_POSTPONED_COUNT,
                                                  MLSL_POSTPONED_DTYPE));
                }
            }
            else
            {
                for (idx = 0; idx < part_count; idx++)
                {

                    MLSL_CALL(mlsl_coll_build_allreduce(part_scheds[idx],
                                                        (char *)coll_param->send_buf + offsets[idx],
                                                        (char *)coll_param->recv_buf + offsets[idx],
                                                        counts[idx],
                                                        dtype,
                                                        coll_param->reduction));
                }
            }
            if (sched->coll_attr.epilogue_fn)
            {
                void* epilogue_in_buf = NULL;
                size_t epilogue_in_count = 0;
                mlsl_datatype_internal_t epilogue_in_dtype = NULL;
                MLSL_CALL(mlsl_sched_sync_schedules(part_scheds, part_count));
                if (sched->coll_attr.prologue_fn)
                {
                    MLSL_CALL(mlsl_sched_add_copy(part_scheds[0],
                                                  &(sched->postponed_fields),
                                                  &(part_scheds[0]->postponed_fields),
                                                  sizeof(mlsl_sched_postponed_fields),
                                                  mlsl_dtype_internal_char));
                    MLSL_CALL(mlsl_sched_add_update_postponed_fields(part_scheds[0], 0, 1));
                    epilogue_in_buf = MLSL_POSTPONED_ADDR;
                    epilogue_in_count = MLSL_POSTPONED_COUNT;
                    epilogue_in_dtype = MLSL_POSTPONED_DTYPE;
                }
                else
                {
                    epilogue_in_buf = coll_param->recv_buf;
                    epilogue_in_count = coll_param->count;
                    epilogue_in_dtype = dtype;
                }
                MLSL_CALL(mlsl_sched_add_epilogue(part_scheds[0],
                                                  sched->coll_attr.epilogue_fn,
                                                  epilogue_in_buf,
                                                  epilogue_in_count,
                                                  epilogue_in_dtype,
                                                  (char *)coll_param->recv_buf,
                                                  &(part_scheds[0]->postponed_fields.count),
                                                  coll_param->count,
                                                  dtype));
            }
            break;
        case mlsl_coll_allgatherv:
            if (coll_param->send_buf != coll_param->recv_buf)
            {
                size_t *copy_counts = static_cast<size_t*>(MLSL_MALLOC(part_count * sizeof(size_t), "copy_counts"));
                size_t *copy_offsets = static_cast<size_t*>(MLSL_MALLOC(part_count * sizeof(size_t), "copy_offsets"));
                for (idx = 0; idx < part_count; idx++)
                {
                    copy_counts[idx] = counts[coll_param->comm->rank] / part_count;
                    copy_offsets[idx] = idx * copy_counts[idx] * dtype_size;
                }
                copy_counts[part_count - 1] += counts[coll_param->comm->rank] % part_count;
                for (idx = 0; idx < part_count; idx++)
                {
                    mlsl_sched_add_copy(part_scheds[idx],
                                        (char *)coll_param->send_buf + copy_offsets[idx],
                                        (char *)coll_param->recv_buf + offsets[coll_param->comm->rank] + copy_offsets[idx],
                                        copy_counts[idx], dtype);
                }
                mlsl_sched_sync_schedules(part_scheds, part_count);
                MLSL_FREE(copy_counts);
                MLSL_FREE(copy_offsets);
            }

            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_bcast(part_scheds[idx],
                                                (char *)coll_param->recv_buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                idx));
            }
            break;
        case mlsl_coll_custom:
            for (idx = 0; idx < part_count; idx++)
                mlsl_sched_adjust_entries(part_scheds[idx], idx, part_count);
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    // TODO: analyze custom sched and insert sync entries
    // MLSL_CALL(mlsl_sched_add_sync(sched));

    MLSL_FREE(counts);
    MLSL_FREE(offsets);

    *scheds = part_scheds;
    *sched_count = part_count;

    return status;
}
