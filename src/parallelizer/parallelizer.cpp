#include "parallelizer/parallelizer.hpp"
#include "sched/entry_factory.hpp"
#include "sched/entry/sycl_copy_device_to_host_entry.hpp"
#include "sched/entry/sycl_copy_host_to_device_entry.hpp"
#include "common/env/env.hpp"

typedef struct
{
    /* keep these 4 fields on the top of structure */
    void* buf;
    size_t count;
    ccl_datatype_t dtype;
    size_t dtype_size;
    /*---*/

    ccl_datatype_internal dtype_internal;
    size_t part_idx;
    size_t part_count;
} ccl_parallelizer_prologue_ctx;

ccl_status_t ccl_parallelizer_prologue_get_buf(const void* ctx, void* field_ptr)
{
    ccl_parallelizer_prologue_ctx* pctx = (ccl_parallelizer_prologue_ctx*)ctx;
    ccl_buffer* buf_ptr = (ccl_buffer*)field_ptr;
    buf_ptr->set(pctx->buf, pctx->count * pctx->dtype_size,
                 pctx->part_idx * (pctx->count / pctx->part_count) * pctx->dtype_size);
    return ccl_status_success;
}

ccl_status_t ccl_parallelizer_prologue_get_count(const void* ctx, void* field_ptr)
{
    ccl_parallelizer_prologue_ctx* pctx = (ccl_parallelizer_prologue_ctx*)ctx;
    size_t count = pctx->count / pctx->part_count;
    if (pctx->part_idx == (pctx->part_count - 1)) count += pctx->count % pctx->part_count;
    size_t* count_ptr = (size_t*)field_ptr;
    *count_ptr = count;
    return ccl_status_success;
}

ccl_status_t ccl_parallelizer_prologue_get_dtype(const void* ctx, void* field_ptr)
{
    ccl_parallelizer_prologue_ctx* pctx = (ccl_parallelizer_prologue_ctx*)ctx;
    ccl_datatype_internal_t* dtype_ptr = (ccl_datatype_internal_t*)field_ptr;
    pctx->dtype_internal.type = pctx->dtype;
    pctx->dtype_internal.size = pctx->dtype_size;
    pctx->dtype_internal.name = "UNKNOWN";
    *dtype_ptr = &pctx->dtype_internal;
    return ccl_status_success;
}

ccl_status_t ccl_parallelizer::process(ccl_sched* sched)
{
    CCL_ASSERT(sched);

    ccl_status_t status = ccl_status_success;
    size_t part_count = 1, idx, base_count, dtype_size;
    ccl_coll_param* coll_param = &(sched->coll_param);
    ccl_datatype_internal_t dtype = coll_param->dtype;
    dtype_size = ccl_datatype_get_size(dtype);
    ccl_coll_type coll_type = coll_param->ctype;

    std::vector<size_t> counts;
    std::vector<size_t> offsets;
    auto& part_scheds = sched->partial_scheds;
    size_t* recv_counts = nullptr;
    sched_entry* e = nullptr;

    std::vector<ccl_buffer> ag_recv_bufs;
    size_t ag_recv_bytes = 0;

    ccl_datatype_internal_t itype = ccl_dtype_internal_none;
    size_t itype_size = 0;

    switch (coll_type)
    {
        case ccl_coll_barrier:
            break;
        case ccl_coll_bcast:
        case ccl_coll_reduce:
        case ccl_coll_allreduce:
            if (coll_param->count * dtype_size <= env_data.max_short_size)
            {
                part_count = 1;
            }
            else
            {
                part_count = max_data_partition_count;
            }
            break;
        case ccl_coll_allgatherv:
            if (env_data.allgatherv_algo == ccl_allgatherv_algo_direct ||
                env_data.allgatherv_algo == ccl_allgatherv_algo_naive)
            {
                part_count = 1;
            }
            else if ((env_data.allgatherv_algo == ccl_allgatherv_algo_multi_bcast) || (env_data.allgatherv_algo == ccl_allgatherv_algo_flat))
            {
                part_count = coll_param->comm->size();
                ag_recv_bufs.resize(part_count);
            }
            else
            {
                CCL_FATAL("unexpected allgatherv_algo ", env_data.allgatherv_algo);
            }
            break;
        case ccl_coll_sparse_allreduce:
            part_count = 1;
            break;
        default:
            CCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }

    LOG_DEBUG("sched ", sched, ", num_entries ", sched->entries.size(),
              ", coll_type ", ccl_coll_type_to_str(coll_type), ", part_count ", part_count);

    counts.resize(part_count, 0);
    offsets.resize(part_count, 0);
    for (idx = 0; idx < part_count; idx++)
    {
        ccl_coll_param part_coll_param{};
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
        case ccl_coll_barrier:
            break;
        case ccl_coll_bcast:
        case ccl_coll_reduce:
        case ccl_coll_allreduce:
        case ccl_coll_sparse_allreduce:
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
        case ccl_coll_allgatherv:
            recv_counts = coll_param->recv_counts;
            CCL_ASSERT(recv_counts);
            counts[0] = recv_counts[0];
            offsets[0] = 0;
            if (env_data.allgatherv_algo == ccl_allgatherv_algo_direct ||
                env_data.allgatherv_algo == ccl_allgatherv_algo_naive)
            {
                for (idx = 0; idx < coll_param->comm->size(); idx++)
                    ag_recv_bytes += recv_counts[idx] * dtype_size;
            }
            else
            {
                ag_recv_bytes = counts[0] * dtype_size;
                for (idx = 1; idx < part_count; idx++)
                {
                    counts[idx] = recv_counts[idx];
                    offsets[idx] = offsets[idx - 1] + counts[idx - 1] * dtype_size;
                    ag_recv_bytes += counts[idx] * dtype_size;
                }
            }
            break;
        default:
            CCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }

    switch (coll_type)
    {
        case ccl_coll_barrier:
            for (idx = 0; idx < part_count; idx++)
            {
                CCL_CALL(ccl_coll_build_barrier(part_scheds[idx].get()));
            }
            break;
        case ccl_coll_bcast:
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {

                if (coll_param->comm->rank() == coll_param->root)
                {
                    entry_factory::make_entry<sycl_copy_device_to_host_entry>(part_scheds[0].get(),
                                                                              coll_param->sycl_buf,
                                                                              coll_param->buf,
                                                                              dtype, coll_param->stream);
                }

                sched->sync_partial_scheds();
            }
#endif /* ENABLE_SYCL */
            for (idx = 0; idx < part_count; idx++)
            {
                CCL_CALL(ccl_coll_build_bcast(part_scheds[idx].get(),
                                              ccl_buffer(&(coll_param->buf),
                                                         coll_param->count * dtype_size,
                                                         offsets[idx],
                                                         ccl_buffer_type::INDIRECT),
                                              counts[idx],
                                              dtype,
                                              coll_param->root));
            }
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {
                sched->sync_partial_scheds();
                entry_factory::make_entry<sycl_copy_host_to_device_entry>(part_scheds[0].get(),
                                                                          coll_param->buf,
                                                                          coll_param->sycl_buf,
                                                                          dtype, coll_param->stream);
            }
#endif /* ENABLE_SYCL */
            break;
        case ccl_coll_reduce:
            for (idx = 0; idx < part_count; idx++)
            {
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {

                entry_factory::make_entry<sycl_copy_device_to_host_entry>(part_scheds[0].get(),
                                                                          coll_param->sycl_send_buf,
                                                                          (void *)coll_param->send_buf,
                                                                          dtype, coll_param->stream);
                sched->sync_partial_scheds();
            }
#endif /* ENABLE_SYCL */
                CCL_CALL(ccl_coll_build_reduce(part_scheds[idx].get(),
                                               ccl_buffer(&(coll_param->send_buf),
                                                          coll_param->count * dtype_size,
                                                          offsets[idx],
                                                          ccl_buffer_type::INDIRECT),
                                               ccl_buffer(&(coll_param->recv_buf),
                                                          coll_param->count * dtype_size,
                                                          offsets[idx],
                                                          ccl_buffer_type::INDIRECT),
                                               counts[idx],
                                               dtype,
                                               coll_param->reduction,
                                               coll_param->root));
            }
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {
                sched->sync_partial_scheds();
                if (coll_param->comm->rank() == coll_param->root)
                {
                    entry_factory::make_entry<sycl_copy_host_to_device_entry>(part_scheds[0].get(),
                                                                              coll_param->recv_buf,
                                                                              coll_param->sycl_recv_buf,
                                                                              dtype, coll_param->stream);
                }
            }
#endif /* ENABLE_SYCL */

            break;
        case ccl_coll_allreduce:
        {
            ccl_parallelizer_prologue_ctx* main_ctx = nullptr;

#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {

                entry_factory::make_entry<sycl_copy_device_to_host_entry>(part_scheds[0].get(),
                                                                          coll_param->sycl_send_buf,
                                                                          (void *)coll_param->send_buf,
                                                                          dtype, coll_param->stream);
                sched->sync_partial_scheds();
            }
#endif /* ENABLE_SYCL */

            if (sched->coll_attr.prologue_fn)
            {
                main_ctx =
                    (ccl_parallelizer_prologue_ctx*)part_scheds[0]->alloc_buffer(sizeof(ccl_parallelizer_prologue_ctx)).get_ptr();
                main_ctx->part_idx = 0;
                main_ctx->part_count = 1;
                entry_factory::make_entry<prologue_entry>(part_scheds[0].get(),
                                                          sched->coll_attr.prologue_fn,
                                                          ccl_buffer(&(coll_param->send_buf),
                                                                     coll_param->count * dtype_size,
                                                                     ccl_buffer_type::INDIRECT),
                                                          coll_param->count,
                                                          dtype,
                                                          &(main_ctx->buf),
                                                          &(main_ctx->count),
                                                          &(main_ctx->dtype),
                                                          &(main_ctx->dtype_size));

                sched->sync_partial_scheds();

                for (idx = 0; idx < part_count; idx++)
                {
                    ccl_parallelizer_prologue_ctx* part_ctx =
                        (ccl_parallelizer_prologue_ctx*)part_scheds[idx]->alloc_buffer(sizeof(ccl_parallelizer_prologue_ctx)).get_ptr();
                    part_ctx->part_idx = idx;
                    part_ctx->part_count = part_count;

                    entry_factory::make_entry<copy_entry>(part_scheds[idx].get(),
                                                          ccl_buffer(main_ctx, sizeof(ccl_parallelizer_prologue_ctx)),
                                                          ccl_buffer(part_ctx, sizeof(ccl_parallelizer_prologue_ctx)),
                                                          sizeof(void*) + sizeof(size_t) +
                                                          sizeof(ccl_datatype_t) + sizeof(size_t),
                                                          ccl_dtype_internal_char);

                    e = entry_factory::make_entry<coll_entry>(part_scheds[idx].get(),
                                                              ccl_coll_allreduce,
                                                              ccl_buffer(), /* send_buf */
                                                              ccl_buffer(), /* recv_buf */
                                                              0, /* count */
                                                              ccl_dtype_internal_none,
                                                              coll_param->reduction);

                   e->set_field_fn(ccl_sched_entry_field_send_buf,
                                   ccl_parallelizer_prologue_get_buf, part_ctx, false);
                   e->set_field_fn(ccl_sched_entry_field_recv_buf,
                                   ccl_parallelizer_prologue_get_buf, part_ctx, false); // in-place
                   e->set_field_fn(ccl_sched_entry_field_cnt,
                                   ccl_parallelizer_prologue_get_count, part_ctx, false);
                   e->set_field_fn(ccl_sched_entry_field_dtype,
                                   ccl_parallelizer_prologue_get_dtype, part_ctx, false);
                }

                if (!sched->coll_attr.epilogue_fn)
                {
                    sched->sync_partial_scheds();

                    e = entry_factory::make_entry<copy_entry>(part_scheds[0].get(),
                                                              ccl_buffer(), /* in_buf */
                                                              ccl_buffer(&(coll_param->recv_buf),
                                                                         coll_param->count * dtype_size,
                                                                         ccl_buffer_type::INDIRECT),
                                                       0, /* count */
                                                       ccl_dtype_internal_none);
                    e->set_field_fn(ccl_sched_entry_field_in_buf,
                                    ccl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(ccl_sched_entry_field_cnt,
                                    ccl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(ccl_sched_entry_field_dtype,
                                    ccl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }
            else
            {
                for (idx = 0; idx < part_count; idx++)
                {
                    CCL_CALL(ccl_coll_build_allreduce(part_scheds[idx].get(),
                                                      ccl_buffer(&(coll_param->send_buf),
                                                                 coll_param->count * dtype_size,
                                                                 offsets[idx],
                                                                 ccl_buffer_type::INDIRECT),
                                                      ccl_buffer(&(coll_param->recv_buf),
                                                                 coll_param->count * dtype_size,
                                                                 offsets[idx],
                                                                 ccl_buffer_type::INDIRECT),
                                                      counts[idx],
                                                      dtype,
                                                      coll_param->reduction));
                }
            }

            if (sched->coll_attr.epilogue_fn)
            {
                sched->sync_partial_scheds();
                e = entry_factory::make_entry<epilogue_entry>(part_scheds[0].get(),
                                                              sched->coll_attr.epilogue_fn,
                                                              ccl_buffer(&(coll_param->recv_buf),
                                                                         coll_param->count * dtype_size,
                                                                         ccl_buffer_type::INDIRECT),
                                                              coll_param->count,
                                                              dtype,
                                                              ccl_buffer(&(coll_param->recv_buf),
                                                                         coll_param->count * dtype_size,
                                                                         ccl_buffer_type::INDIRECT),
                                                              coll_param->count,
                                                              dtype);
                if (sched->coll_attr.prologue_fn)
                {
                    e->set_field_fn(ccl_sched_entry_field_in_buf,
                                    ccl_parallelizer_prologue_get_buf, main_ctx, false);
                    e->set_field_fn(ccl_sched_entry_field_in_cnt,
                                    ccl_parallelizer_prologue_get_count, main_ctx, false);
                    e->set_field_fn(ccl_sched_entry_field_in_dtype,
                                    ccl_parallelizer_prologue_get_dtype, main_ctx, false);
                }
            }

#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {
                sched->sync_partial_scheds();
                entry_factory::make_entry<sycl_copy_host_to_device_entry>(part_scheds[0].get(),
                                                                          coll_param->recv_buf,
                                                                          coll_param->sycl_recv_buf,
                                                                          dtype, coll_param->stream);
            }
#endif /* ENABLE_SYCL */

            break;
        }
        case ccl_coll_allgatherv:
        {
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {
                entry_factory::make_entry<sycl_copy_device_to_host_entry>(part_scheds[0].get(),
                                                                          coll_param->sycl_send_buf,
                                                                          (void*)coll_param->send_buf,
                                                                          dtype, coll_param->stream);
                sched->sync_partial_scheds();
            }
#endif /* ENABLE_SYCL */
            if (env_data.allgatherv_algo == ccl_allgatherv_algo_direct ||
                env_data.allgatherv_algo == ccl_allgatherv_algo_naive)
            {
                CCL_CALL(ccl_coll_build_allgatherv(part_scheds[0].get(),
                                                   ccl_buffer(&(coll_param->send_buf),
                                                              coll_param->send_count * dtype_size,
                                                              ccl_buffer_type::INDIRECT),
                                                   coll_param->send_count,
                                                   ccl_buffer(&(coll_param->recv_buf),
                                                              ag_recv_bytes,
                                                              ccl_buffer_type::INDIRECT),
                                                   coll_param->recv_counts,
                                                   dtype));
            }
            else
            {
                for (idx = 0; idx < coll_param->comm->size(); idx++)
                {
                    if (env_data.enable_allgatherv_iov)
                    {
                        ag_recv_bufs[idx].set(&(((char**)coll_param->recv_buf)[idx]),
                                              counts[idx] * dtype_size,
                                              ccl_buffer_type::INDIRECT);
                    }
                    else
                    {
                        ag_recv_bufs[idx].set(&(coll_param->recv_buf),
                                              ag_recv_bytes,
                                              offsets[idx],
                                              ccl_buffer_type::INDIRECT);
                    }
                }

                if (env_data.allgatherv_algo == ccl_allgatherv_algo_flat)
                {
                    auto send_seg = ccl_buffer(&(coll_param->send_buf), coll_param->send_count * dtype_size, ccl_buffer_type::INDIRECT);
                    size_t my_rank = coll_param->comm->rank();
                    if (coll_param->send_buf != coll_param->recv_buf)
                    {
                        entry_factory::make_entry<copy_entry>(part_scheds[2 * my_rank % part_count].get(),
                                                              ccl_buffer(&(coll_param->send_buf),
                                                                         coll_param->send_count * dtype_size,
                                                                         ccl_buffer_type::INDIRECT),
                                                              ag_recv_bufs[my_rank],
                                                              counts[my_rank], dtype);

                    }
                    else
                    {
                        send_seg = ccl_buffer(&(coll_param->send_buf), coll_param->send_count * dtype_size, offsets[my_rank], ccl_buffer_type::INDIRECT);
                    }

                    for (idx = 0; idx < part_count; idx++)
                    {
                         if (idx == my_rank) continue;

                         entry_factory::make_entry<recv_entry>(part_scheds[(my_rank+idx) % part_count].get(),
                                                               ag_recv_bufs[idx],
                                                               counts[idx],
                                                               dtype, 
                                                               idx);
                         entry_factory::make_entry<send_entry>(part_scheds[(my_rank+idx) % part_count].get(),
                                                               send_seg,
                                                               counts[my_rank],
                                                               dtype,
                                                               idx);
                    }
                    sched->sync_partial_scheds();

                }
                else
                {
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
                            entry_factory::make_entry<copy_entry>(part_scheds[idx].get(),
                                                                  ccl_buffer(&(coll_param->send_buf),
                                                                             coll_param->send_count * dtype_size,
                                                                             copy_offsets[idx],
                                                                             ccl_buffer_type::INDIRECT),
                                                                  ag_recv_bufs[coll_param->comm->rank()] + copy_offsets[idx],
                                                                  copy_counts[idx], dtype);
                        }
                        sched->sync_partial_scheds();
                    }

                    for (idx = 0; idx < part_count; idx++)
                    {
                        CCL_CALL(ccl_coll_build_bcast(part_scheds[idx].get(),
                                                      ag_recv_bufs[idx],
                                                      counts[idx],
                                                      dtype,
                                                      idx));
                    }
                }
            }
#ifdef ENABLE_SYCL
            /* convert sycl buffer */
            if (coll_param->stream && (ccl_stream_type_t)(coll_param->stream->get_type()) == ccl_stream_sycl)
            {
                sched->sync_partial_scheds();
                entry_factory::make_entry<sycl_copy_host_to_device_entry>(part_scheds[0].get(),
                                                                          coll_param->recv_buf,
                                                                          coll_param->sycl_recv_buf,
                                                                          dtype, coll_param->stream);
            }
#endif /* ENABLE_SYCL */
            break;
        }
        case ccl_coll_sparse_allreduce:
            itype = coll_param->sparse_param.itype;
            itype_size = ccl_datatype_get_size(itype);
            for (idx = 0; idx < part_count; idx++)
            {
                CCL_CALL(ccl_coll_build_sparse_allreduce(part_scheds[idx].get(),
                                                         ccl_buffer(&(coll_param->send_buf),
                                                                    coll_param->send_count * itype_size,
                                                                    offsets[idx],
                                                                    ccl_buffer_type::INDIRECT),
                                                         coll_param->send_count,
                                                         ccl_buffer(&(coll_param->sparse_param.snd_val_buf),
                                                                    coll_param->sparse_param.snd_val_count * dtype_size,
                                                                    offsets[idx],
                                                                    ccl_buffer_type::INDIRECT),
                                                         coll_param->sparse_param.snd_val_count,
                                                         ccl_buffer(&(coll_param->sparse_param.rcv_ind_buf),
                                                                    //*(coll_param.recv_counts) * itype_size,
                                                                    -1, /* unknown size */
                                                                    offsets[idx],
                                                                    ccl_buffer_type::INDIRECT),
                                                         coll_param->recv_counts,
                                                         ccl_buffer(&(coll_param->sparse_param.rcv_val_buf),
                                                                    //*(coll_param.sparse_param.rcv_val_count) * dtype_size,
                                                                    -1, /* unknown size */
                                                                    offsets[idx],
                                                                    ccl_buffer_type::INDIRECT),
                                                         coll_param->sparse_param.rcv_val_count,
                                                         coll_param->sparse_param.itype,
                                                         dtype,
                                                         coll_param->reduction));
            }
            break;

        default:
            CCL_FATAL("unexpected coll_type ", coll_type);
            break;
    }
    return status;
}
