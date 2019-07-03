#include "parallelizer/parallelizer.hpp"
#include "sched/entry_factory.hpp"
#include "common/env/env.hpp"

#define ICCL_MIN_PART_SIZE (2048)

typedef struct
{
    /* keep these 4 fields on the top of structure */
    void* buf;
    size_t count;
    iccl_datatype_t dtype;
    size_t dtype_size;
    /*---*/

    iccl_datatype_internal dtype_internal;
    size_t part_idx;
    size_t part_count;
} iccl_parallelizer_prologue_ctx;

iccl_status_t iccl_parallelizer_prologue_get_buf(const void* ctx, void* field_ptr)
{
    iccl_parallelizer_prologue_ctx* pctx = (iccl_parallelizer_prologue_ctx*)ctx;
    void** buf_ptr = (void**)field_ptr;
    *buf_ptr = (char*)pctx->buf + pctx->part_idx * (pctx->count / pctx->part_count) * pctx->dtype_size;
    return iccl_status_success;
}

iccl_status_t iccl_parallelizer_prologue_get_count(const void* ctx, void* field_ptr)
{
    iccl_parallelizer_prologue_ctx* pctx = (iccl_parallelizer_prologue_ctx*)ctx;
    size_t count = pctx->count / pctx->part_count;
    if (pctx->part_idx == (pctx->part_count - 1)) count += pctx->count % pctx->part_count;
    size_t* count_ptr = (size_t*)field_ptr;
    *count_ptr = count;
    return iccl_status_success;
}

iccl_status_t iccl_parallelizer_prologue_get_dtype(const void* ctx, void* field_ptr)
{
    iccl_parallelizer_prologue_ctx* pctx = (iccl_parallelizer_prologue_ctx*)ctx;
    iccl_datatype_internal_t* dtype_ptr = (iccl_datatype_internal_t*)field_ptr;
    pctx->dtype_internal.type = pctx->dtype;
    pctx->dtype_internal.size = pctx->dtype_size;
    pctx->dtype_internal.name = "UNKNOWN";
    *dtype_ptr = &pctx->dtype_internal;
    return iccl_status_success;
}

iccl_status_t iccl_parallelizer::process(iccl_sched* sched)
{
    ICCL_ASSERT(sched);

    iccl_status_t status = iccl_status_success;
    size_t part_count = 1, idx, base_count, dtype_size;
    iccl_coll_param* coll_param = &(sched->coll_param);
    iccl_datatype_internal_t dtype = coll_param->dtype;
    dtype_size = iccl_datatype_get_size(dtype);
    iccl_coll_type coll_type = coll_param->ctype;

    std::vector<size_t> counts;
    std::vector<size_t> offsets;
    auto& part_scheds = sched->partial_scheds;
    size_t* recv_counts = nullptr;
    std::shared_ptr<sched_entry> e;

    std::vector<char*> ag_recv_bufs;

    switch (coll_type)
    {
        case iccl_coll_barrier:
        case iccl_coll_sparse_allreduce:
            part_count = 1;
            break;
        case iccl_coll_bcast:
        case iccl_coll_reduce:
        case iccl_coll_allreduce:
            if (coll_param->count * dtype_size <= ICCL_MIN_PART_SIZE)
            {
                part_count = 1;
            }
            else
            {
                part_count = max_data_partition_count;
            }
            break;
        case iccl_coll_allgatherv:
            if (env_data.allgatherv_algo == iccl_allgatherv_algo_direct)
            {
                part_count = 1;
            }
            else
            {
                part_count = coll_param->comm->size();
                ag_recv_bufs.resize(part_count);
            }
            break;
        default:
            ICCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }

    LOG_DEBUG("sched ", sched, ", num_entries ", sched->entries.size(), ", coll_type ", coll_type, ", part_count ", part_count);

    counts.resize(part_count, 0);
    offsets.resize(part_count, 0);
    for (idx = 0; idx < part_count; idx++)
    {
        iccl_coll_param part_coll_param{};
        part_coll_param.comm = sched->coll_param.comm;
        part_coll_param.ctype = sched->coll_param.ctype;
        part_coll_param.dtype = sched->coll_param.dtype;
        sched->add_partial_sched(part_coll_param);
    }

    for (idx = 0; idx < part_count; idx++)
    {
        /* in this place all coll attributes for partial schedules
         * are taken from master schedule, including priority */
        part_scheds[idx]->coll_attr = sched->coll_attr;
    }

    switch (coll_type)
    {
        case iccl_coll_barrier:
        case iccl_coll_sparse_allreduce:
            break;
        case iccl_coll_bcast:
        case iccl_coll_reduce:
        case iccl_coll_allreduce:
            base_count = coll_param->count / part_count;
            for (idx = 0; idx < part_count; idx++)
            {
                counts[idx] = base_count;
                offsets[idx] = idx * counts[idx] * dtype_size;
            }
            counts[part_count - 1] += coll_param->count % part_count;
            for (idx = 0; idx < part_count; idx++)
            {
                part_scheds[idx]->coll_param.count = counts[idx];
            }
            break;
        case iccl_coll_allgatherv:
            recv_counts = coll_param->recv_counts;
            ICCL_ASSERT(recv_counts);
            counts[0] = recv_counts[0];
            offsets[0] = 0;
            for (idx = 1; idx < part_count; idx++)
            {
                counts[idx] = recv_counts[idx];
                offsets[idx] = offsets[idx - 1] + counts[idx - 1] * dtype_size;
            }
            break;
        default:
            ICCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }

    switch (coll_type)
    {
        case iccl_coll_barrier:
            for (idx = 0; idx < part_count; idx++)
            {
                ICCL_CALL(iccl_coll_build_barrier(part_scheds[idx].get()));
            }
            break;
        case iccl_coll_bcast:
            for (idx = 0; idx < part_count; idx++)
            {
                ICCL_CALL(iccl_coll_build_bcast(part_scheds[idx].get(),
                                                (char*) coll_param->buf + offsets[idx],
                                                counts[idx],
                                                dtype,
                                                coll_param->root));
            }
            break;
        case iccl_coll_reduce:
            for (idx = 0; idx < part_count; idx++)
            {
                ICCL_CALL(iccl_coll_build_reduce(part_scheds[idx].get(),
                                                 (char*) coll_param->send_buf + offsets[idx],
                                                 (char*) coll_param->recv_buf + offsets[idx],
                                                 counts[idx],
                                                 dtype,
                                                 coll_param->reduction,
                                                 coll_param->root));
            }
            break;
        case iccl_coll_allreduce:
        {
            iccl_parallelizer_prologue_ctx* main_ctx = nullptr;
            if (sched->coll_attr.prologue_fn)
            {
                main_ctx = (iccl_parallelizer_prologue_ctx*)part_scheds[0]->alloc_buffer(sizeof(iccl_parallelizer_prologue_ctx));
                main_ctx->part_idx = 0;
                main_ctx->part_count = 1;
                entry_factory::make_prologue_entry(part_scheds[0].get(),
                                                   sched->coll_attr.prologue_fn,
                                                   (char*) coll_param->send_buf,
                                                   coll_param->count,
                                                   dtype,
                                                   &(main_ctx->buf),
                                                   &(main_ctx->count),
                                                   &(main_ctx->dtype),
                                                   &(main_ctx->dtype_size));

                sched->sync_partial_scheds();

                for (idx = 0; idx < part_count; idx++)
                {
                    iccl_parallelizer_prologue_ctx* part_ctx =
                        (iccl_parallelizer_prologue_ctx*)part_scheds[idx]->alloc_buffer(sizeof(iccl_parallelizer_prologue_ctx));
                    part_ctx->part_idx = idx;
                    part_ctx->part_count = part_count;
                    entry_factory::make_copy_entry(part_scheds[idx].get(),
                                                   main_ctx,
                                                   part_ctx,
                                                   sizeof(void*) + sizeof(size_t) +
                                                   sizeof(iccl_datatype_t) + sizeof(size_t),
                                                   iccl_dtype_internal_char);
                    e = entry_factory::make_coll_entry(part_scheds[idx].get(),
                                                       iccl_coll_allreduce,
                                                       nullptr, /* send_buf */
                                                       nullptr, /* recv_buf */
                                                       0, /* count */
                                                       iccl_dtype_internal_none,
                                                       coll_param->reduction);
                    e->set_field_fn(iccl_sched_entry_field_send_buf,
                                    iccl_parallelizer_prologue_get_buf, part_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_recv_buf,
                                    iccl_parallelizer_prologue_get_buf, part_ctx, false); // in-place
                    e->set_field_fn(iccl_sched_entry_field_cnt,
                                    iccl_parallelizer_prologue_get_count, part_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_dtype,
                                    iccl_parallelizer_prologue_get_dtype, part_ctx, false);
                }
                if (!sched->coll_attr.epilogue_fn)
                {
                    sched->sync_partial_scheds();

                    e = entry_factory::make_copy_entry(part_scheds[0].get(),
                                                       nullptr, /* in_buf */
                                                       coll_param->recv_buf,
                                                       0, /* count */
                                                       iccl_dtype_internal_none);
                    e->set_field_fn(iccl_sched_entry_field_in_buf,
                                    iccl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_cnt,
                                    iccl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_dtype,
                                    iccl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }
            else
            {
                for (idx = 0; idx < part_count; idx++)
                {
                    ICCL_CALL(iccl_coll_build_allreduce(part_scheds[idx].get(),
                                                        (char*) coll_param->send_buf + offsets[idx],
                                                        (char*) coll_param->recv_buf + offsets[idx],
                                                        counts[idx],
                                                        dtype,
                                                        coll_param->reduction));
                }
            }
            if (sched->coll_attr.epilogue_fn)
            {
                sched->sync_partial_scheds();
                e = entry_factory::make_epilogue_entry(part_scheds[0].get(),
                                                       sched->coll_attr.epilogue_fn,
                                                       (char*) coll_param->recv_buf,
                                                       coll_param->count,
                                                       dtype,
                                                       (char*) coll_param->recv_buf,
                                                       coll_param->count,
                                                       dtype);
                if (sched->coll_attr.prologue_fn)
                {
                    e->set_field_fn(iccl_sched_entry_field_in_buf,
                                    iccl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_in_cnt,
                                    iccl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(iccl_sched_entry_field_in_dtype,
                                    iccl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }
            break;
        }
        case iccl_coll_allgatherv:
            if (env_data.allgatherv_algo == iccl_allgatherv_algo_direct)
            {
                ICCL_CALL(iccl_coll_build_allgatherv(part_scheds[idx].get(),
                                                    (char*) coll_param->send_buf,
                                                    (char*) coll_param->recv_buf,
                                                    coll_param->send_count,
                                                    coll_param->recv_counts,
                                                    dtype));
            }
            else
            {
                for (size_t idx = 0; idx < coll_param->comm->size(); idx++)
                {
                    ag_recv_bufs[idx] = (env_data.vector_allgatherv) ?
                        ((char**)coll_param->recv_buf)[idx] :
                        (char*)coll_param->recv_buf + offsets[idx];
                }
                if (coll_param->send_buf != coll_param->recv_buf)
                {
                    std::vector<size_t> copy_counts(part_count);
                    std::vector<size_t> copy_offsets(part_count);
                    for (idx = 0; idx < part_count; idx++)
                    {
                        copy_counts[idx] = counts[coll_param->comm->rank()] / part_count;
                        copy_offsets[idx] = idx * copy_counts[idx] * dtype_size;
                    }
                    copy_counts[part_count - 1] += counts[coll_param->comm->rank()] % part_count;
                    for (idx = 0; idx < part_count; idx++)
                    {
                        entry_factory::make_copy_entry(part_scheds[idx].get(),
                                                       (char*) coll_param->send_buf +
                                                       copy_offsets[idx],
                                                       ag_recv_bufs[coll_param->comm->rank()] +
                                                       copy_offsets[idx],
                                                       copy_counts[idx], dtype);
                    }

                    sched->sync_partial_scheds();
                }

                for (idx = 0; idx < part_count; idx++)
                {
                    ICCL_CALL(iccl_coll_build_bcast(part_scheds[idx].get(),
                                                    ag_recv_bufs[idx],
                                                    counts[idx],
                                                    dtype,
                                                    idx));
                }
            }
            break;

        case iccl_coll_sparse_allreduce:
            for (idx = 0; idx < part_count; idx++)
            {
                ICCL_CALL(iccl_coll_build_sparse_allreduce(part_scheds[idx].get(),
                                                           coll_param->send_buf,
                                                           coll_param->send_count,
                                                           coll_param->sparse_param.snd_val_buf,
                                                           coll_param->sparse_param.snd_val_count,
                                                           coll_param->sparse_param.rcv_ind_buf,
                                                           coll_param->recv_counts,
                                                           coll_param->sparse_param.rcv_val_buf,
                                                           coll_param->sparse_param.rcv_val_count,
                                                           coll_param->sparse_param.itype,
                                                           dtype,
                                                           coll_param->reduction));
            }
            break;

        default:
            ICCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }

    return status;
}
