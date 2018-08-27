#include "comp.h"
#include "parallelizer.h"

#define MLSL_MIN_PART_SIZE (2048)

mlsl_parallelizer *global_parallelizer = NULL;

mlsl_status_t mlsl_parallelizer_create(size_t partition_count, mlsl_parallelizer **parallelizer)
{
    mlsl_parallelizer *p = MLSL_CALLOC(sizeof(mlsl_parallelizer), "parallelizer");
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
    MLSL_ASSERT(sched && sched->coll_desc);

    mlsl_status_t status = mlsl_status_success;
    size_t part_count = 1, idx, base_count, dtype_size;
    mlsl_coll_desc *desc = sched->coll_desc;
    mlsl_data_type_t dtype = desc->dtype;
    dtype_size = mlsl_get_dtype_size(dtype);
    mlsl_coll_type coll_type = desc->ctype;

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
            if (desc->count * dtype_size <= MLSL_MIN_PART_SIZE)
                part_count = 1;
            else
                part_count = parallelizer->partition_count;
            break;
        case mlsl_coll_allgatherv:
            part_count = desc->comm->proc_count;
            break;
        default:
            MLSL_ASSERT_FMT(0, "unexpected coll_type %d", coll_type);
            break;
    }

    MLSL_LOG(DEBUG, "sched %p, num_entries %zu, size %zu, coll_type %d, part_count %zu",
             sched, sched->num_entries, sched->size, coll_type, part_count);

    part_scheds = MLSL_MALLOC(sizeof(mlsl_sched*) * part_count, "scheds");
    if (coll_type == mlsl_coll_custom)
    {
        for (idx = 0; idx < part_count; idx++)
            mlsl_sched_clone(sched, &part_scheds[idx]);
    }
    else
    {
        counts = MLSL_MALLOC(sizeof(size_t) * part_count, "counts");
        offsets = MLSL_MALLOC(sizeof(size_t) * part_count, "counts");
        for (idx = 0; idx < part_count; idx++)
            MLSL_CALL(mlsl_sched_create(&(part_scheds[idx])));
    }

    switch (coll_type)
    {
        case mlsl_coll_barrier:
            break;
        case mlsl_coll_bcast:
        case mlsl_coll_reduce:
        case mlsl_coll_allreduce:
            base_count = desc->count / part_count;
            for (idx = 0; idx < part_count; idx++)
            {
                counts[idx] = base_count;
                offsets[idx] = idx * counts[idx] * dtype_size;
            }
            counts[part_count - 1] += desc->count % part_count;
            break;
        case mlsl_coll_allgatherv:
            recv_counts = desc->recv_counts;
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
                                                (char *)desc->buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                desc->root));
            }
            break;
        case mlsl_coll_reduce:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_reduce(part_scheds[idx],
                                                 (char *)desc->send_buf + offsets[idx],
                                                 (char *)desc->recv_buf + offsets[idx],
                                                 counts[idx],
                                                 dtype,
                                                 desc->reduction,
                                                 desc->root));
            }
            break;
        case mlsl_coll_allreduce:
            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_allreduce(part_scheds[idx],
                                                    (char *)desc->send_buf + offsets[idx],
                                                    (char *)desc->recv_buf + offsets[idx],
                                                    counts[idx],
                                                    dtype,
                                                    desc->reduction));
            }
            break;
        case mlsl_coll_allgatherv:
            if (desc->send_buf != desc->recv_buf)
            {
                size_t *copy_counts = MLSL_MALLOC(part_count * sizeof(size_t), "copy_counts");
                size_t *copy_offsets = MLSL_MALLOC(part_count * sizeof(size_t), "copy_offsets");
                for (idx = 0; idx < part_count; idx++)
                {
                    copy_counts[idx] = counts[desc->comm->proc_idx] / part_count;
                    copy_offsets[idx] = idx * copy_counts[idx] * dtype_size;
                }
                copy_counts[part_count - 1] += counts[desc->comm->proc_idx] % part_count;
                for (idx = 0; idx < part_count; idx++)
                {
                    mlsl_sched_add_copy(part_scheds[idx],
                                        (char *)desc->send_buf + copy_offsets[idx],
                                        (char *)desc->recv_buf + offsets[desc->comm->proc_idx] + copy_offsets[idx],
                                        copy_counts[idx], dtype);
                }
                mlsl_sched_sync_schedules(part_scheds, part_count);
                MLSL_FREE(copy_counts);
                MLSL_FREE(copy_offsets);
            }

            for (idx = 0; idx < part_count; idx++)
            {
                MLSL_CALL(mlsl_coll_build_bcast(part_scheds[idx],
                                                (char *)desc->recv_buf + offsets[idx],
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
