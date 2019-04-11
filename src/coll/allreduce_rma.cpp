#include "coll/allreduce_rma.hpp"
#include "coll/coll_algorithms.hpp"
#include "sched/entry_factory.hpp"
#include "exec/exec.hpp"

mlsl_status_t rma_ring_allreduce_reset_sync_flag(const void* ctx)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    ar_handler->sync_flag = 0;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_reset_dst_ready_flag(const void* ctx)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    ar_handler->dst_ready_flag = 0;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_remote_sync_flag_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = &(ar_handler->remote_sync_flag_mr);
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_sync_flag_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->sync_flag_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_sync_flags_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->sync_flags_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_send_buf_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->send_buf_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_recv_buf_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->recv_buf_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_tmp_buf_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->tmp_buf_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_dst_ready_flag_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->dst_ready_flag_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_dst_ready_value_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = ar_handler->dst_ready_value_mr;
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_remote_dst_ready_flag_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = &(ar_handler->remote_dst_ready_flag_mr);
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_remote_rs_dst_buf_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = &(ar_handler->remote_rs_dst_buf_mr);
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t rma_ring_allreduce_get_remote_recv_buf_mr(const void* ctx, void* field_ptr)
{
    mlsl_rma_ring_allreduce_handler* ar_handler = (mlsl_rma_ring_allreduce_handler*)ctx;
    atl_mr_t* mr = &(ar_handler->remote_recv_buf_mr);
    atl_mr_t** mr_ptr = (atl_mr_t**)field_ptr;
    *mr_ptr = mr;
    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_build_ring_rma_allreduce(mlsl_sched* sched, const void* send_buf, void* recv_buf,
                                                 size_t count, mlsl_datatype_internal_t dtype, mlsl_reduction_t op)
{
    int inplace = (send_buf == recv_buf) ? 1 : 0;
    LOG_DEBUG("build ring rma allreduce (", (inplace) ? "in-place" : "out-of-place", ")");

    mlsl_status_t status = mlsl_status_success;
    size_t comm_size, rank;
    mlsl_comm *comm = sched->coll_param.comm;
    size_t dtype_size = mlsl_datatype_get_size(dtype);
    size_t idx = 0;
    void* tmp_buf = NULL;
    std::shared_ptr<sched_entry> e;

    comm_size = comm->size();
    rank = comm->rank();

    MLSL_THROW_IF_NOT(sched && send_buf && recv_buf, "incorrect values, sched ", sched,
                    ", send ", send_buf, ", recv ", recv_buf);

    if (comm_size == 1)
    {
        if (!inplace) {
            entry_factory::make_copy_entry(sched, send_buf, recv_buf, count, dtype);
            sched->add_barrier();
        }
        return mlsl_status_success;
    }

    mlsl_rma_ring_allreduce_handler* ar_handler =
        (mlsl_rma_ring_allreduce_handler*)sched->alloc_buffer(sizeof(mlsl_rma_ring_allreduce_handler));
    ar_handler->sync_flags = (uint64_t*)sched->alloc_buffer(2 * comm_size * sizeof(uint64_t));

    sched->set_entry_exec_mode(mlsl_sched_entry_exec_once);

    entry_factory::make_register_entry(sched, 2 * comm_size * sizeof(uint64_t), ar_handler->sync_flags, &ar_handler->sync_flags_mr);
    entry_factory::make_register_entry(sched, sizeof(uint64_t), (void*)&ar_handler->sync_flag, &ar_handler->sync_flag_mr);
    entry_factory::make_register_entry(sched, sizeof(uint64_t), (void*)&ar_handler->dst_ready_flag, &ar_handler->dst_ready_flag_mr);
    entry_factory::make_register_entry(sched, sizeof(uint64_t), (void*)&ar_handler->dst_ready_value, &ar_handler->dst_ready_value_mr);

    if (inplace)
    {
        tmp_buf = sched->alloc_buffer(count * dtype_size);
        entry_factory::make_register_entry(sched, count * dtype_size, (void*)tmp_buf, &ar_handler->tmp_buf_mr);
    }
    else
        entry_factory::make_register_entry(sched, count * dtype_size, (void*)send_buf, &ar_handler->send_buf_mr);
    entry_factory::make_register_entry(sched, count * dtype_size, (void*)recv_buf, &ar_handler->recv_buf_mr);

    sched->set_entry_exec_mode(mlsl_sched_entry_exec_regular);

    ar_handler->wait_dst = 1;
    for (idx = 0; idx < 2 * comm_size; idx++)
        ar_handler->sync_flags[idx] = idx + 1;
    ar_handler->dst_ready_flag = 0;
    ar_handler->src_peer = (comm_size + rank - 1) % comm_size;
    ar_handler->dst_peer = (comm_size + rank + 1) % comm_size;

    entry_factory::make_function_entry(sched, rma_ring_allreduce_reset_sync_flag, ar_handler);
    sched->add_barrier();

    sched->set_entry_exec_mode(mlsl_sched_entry_exec_once);

    if (inplace)
    {
        e = entry_factory::make_send_entry(sched, ar_handler->tmp_buf_mr, sizeof(atl_mr_t),
                                       mlsl_dtype_internal_char, ar_handler->src_peer);
        e->set_field_fn(mlsl_sched_entry_field_buf,
                    rma_ring_allreduce_get_tmp_buf_mr,
                    ar_handler);
    }
    else
    {
        e = entry_factory::make_send_entry(sched, ar_handler->recv_buf_mr, sizeof(atl_mr_t),
                                       mlsl_dtype_internal_char, ar_handler->src_peer);
        e->set_field_fn(mlsl_sched_entry_field_buf,
                    rma_ring_allreduce_get_recv_buf_mr,
                    ar_handler);
    }
    e = entry_factory::make_send_entry(sched, ar_handler->recv_buf_mr, sizeof(atl_mr_t),
                                   mlsl_dtype_internal_char, ar_handler->src_peer);
    e->set_field_fn(mlsl_sched_entry_field_buf,
                    rma_ring_allreduce_get_recv_buf_mr,
                    ar_handler);

    e = entry_factory::make_send_entry(sched, ar_handler->sync_flag_mr, sizeof(atl_mr_t),
                                       mlsl_dtype_internal_char, ar_handler->src_peer);
    e->set_field_fn(mlsl_sched_entry_field_buf,
                    rma_ring_allreduce_get_sync_flag_mr,
                    ar_handler);

    entry_factory::make_recv_entry(sched, &ar_handler->remote_rs_dst_buf_mr, sizeof(atl_mr_t),
                                   mlsl_dtype_internal_char, ar_handler->dst_peer);
    entry_factory::make_recv_entry(sched, &ar_handler->remote_recv_buf_mr, sizeof(atl_mr_t),
                                   mlsl_dtype_internal_char, ar_handler->dst_peer);
    entry_factory::make_recv_entry(sched, &ar_handler->remote_sync_flag_mr, sizeof(atl_mr_t),
                                   mlsl_dtype_internal_char, ar_handler->dst_peer);

    if (ar_handler->wait_dst)
    {
        e = entry_factory::make_send_entry(sched, ar_handler->dst_ready_flag_mr, sizeof(atl_mr_t),
                                       mlsl_dtype_internal_char, ar_handler->dst_peer);
        e->set_field_fn(mlsl_sched_entry_field_buf,
                    rma_ring_allreduce_get_dst_ready_flag_mr,
                    ar_handler);
        entry_factory::make_recv_entry(sched, &ar_handler->remote_dst_ready_flag_mr, sizeof(atl_mr_t),
                                       mlsl_dtype_internal_char, ar_handler->src_peer);
    }
    sched->add_barrier();

    sched->set_entry_exec_mode(mlsl_sched_entry_exec_regular);

    if (ar_handler->wait_dst)
    {
        /* let src side to know that this rank (i.e. dst for src rank) is ready for write ops */
        ar_handler->dst_ready_value = 1;
        e = entry_factory::make_write_entry(sched,
                                            &ar_handler->dst_ready_value,
                                            (atl_mr_t*)NULL, /* src_mr */
                                            sizeof(uint64_t),
                                            mlsl_dtype_internal_char,
                                            ar_handler->src_peer,
                                            (atl_mr_t*)NULL, /* dst_mr */
                                            0 /* dst_buf_offset */);
        e->set_field_fn(mlsl_sched_entry_field_src_mr,
                        rma_ring_allreduce_get_dst_ready_value_mr,
                        ar_handler);
        e->set_field_fn(mlsl_sched_entry_field_dst_mr,
                        rma_ring_allreduce_get_remote_dst_ready_flag_mr,
                        ar_handler);

        /* wait when dst side will be ready for write ops */
        entry_factory::make_wait_value_entry(sched, &(ar_handler->dst_ready_flag), 1,
                                             mlsl_condition_equal);

        /* reset dst_ready_flag for next allreduce call */
        entry_factory::make_function_entry(sched, rma_ring_allreduce_reset_dst_ready_flag, ar_handler);
    }

    size_t block_idx = rank;
    size_t main_block_count = count / comm_size;
    size_t buf_offset;

    /* reduce-scatter */
    for (idx = 0; idx < (comm_size - 1); idx++)
    {
        size_t block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += count % comm_size;
        buf_offset = main_block_count * dtype_size * block_idx;

        void* src_buf;
        if (inplace)
            src_buf = recv_buf;
        else
            src_buf = (idx == 0) ? (void*)send_buf : recv_buf;

        e = entry_factory::make_write_entry(sched,
                                            (char*)src_buf + buf_offset,
                                            (atl_mr_t*)NULL, /* src_mr */
                                            block_count,
                                            dtype,
                                            ar_handler->dst_peer,
                                            (atl_mr_t*)NULL, /* dst_mr */
                                            buf_offset);
        e->set_field_fn(mlsl_sched_entry_field_src_mr,
                        (inplace) ?
                            rma_ring_allreduce_get_recv_buf_mr :
                            ((idx == 0) ? rma_ring_allreduce_get_send_buf_mr : rma_ring_allreduce_get_recv_buf_mr),
                        ar_handler);
        e->set_field_fn(mlsl_sched_entry_field_dst_mr,
                        rma_ring_allreduce_get_remote_rs_dst_buf_mr,
                        ar_handler);

        if (block_count * mlsl_datatype_get_size(dtype) > global_data.executor->max_order_waw_size)
            sched->add_barrier();

        e = entry_factory::make_write_entry(sched,
                                            &ar_handler->sync_flags[idx],
                                            (atl_mr_t*)NULL,  /* src_mr */
                                            sizeof(uint64_t),
                                            mlsl_dtype_internal_char,
                                            ar_handler->dst_peer,
                                            (atl_mr_t*)NULL, /* dst_mr */
                                            0 /* dst_buf_offset */);
        e->set_field_fn(mlsl_sched_entry_field_src_mr,
                        rma_ring_allreduce_get_sync_flags_mr,
                        ar_handler);
        e->set_field_fn(mlsl_sched_entry_field_dst_mr,
                        rma_ring_allreduce_get_remote_sync_flag_mr,
                        ar_handler);

        block_idx = (block_idx + comm_size - 1) % comm_size;
        block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += count % comm_size;
        buf_offset = main_block_count * dtype_size * block_idx;

        entry_factory::make_wait_value_entry(sched, &(ar_handler->sync_flag), (idx + 1),
                                             mlsl_condition_greater_or_equal);

        const void* reduce_in_buf = (inplace) ? (const void*)tmp_buf : send_buf;
        void* reduce_inout_buf = recv_buf;
        entry_factory::make_reduce_entry(sched, (char*)reduce_in_buf + buf_offset, block_count,
                                         (char*)reduce_inout_buf + buf_offset, NULL, dtype, op);
    }

    /* allgather */
    size_t flag_idx_offset = (comm_size - 1);
    for (idx = 0; idx < (comm_size - 1); idx++)
    {
        size_t block_count = main_block_count;
        if (block_idx == (comm_size - 1))
            block_count += count % comm_size;
        buf_offset = main_block_count * dtype_size * block_idx;

        void* src_buf = recv_buf;
        e = entry_factory::make_write_entry(sched,
                                            (char*)src_buf + buf_offset,
                                            (atl_mr_t*)NULL, /* src_mr */
                                            block_count,
                                            dtype,
                                            ar_handler->dst_peer,
                                            (atl_mr_t*)NULL, /* dst_mr */
                                            buf_offset);
        e->set_field_fn(mlsl_sched_entry_field_src_mr,
                        rma_ring_allreduce_get_recv_buf_mr,
                        ar_handler);
        e->set_field_fn(mlsl_sched_entry_field_dst_mr,
                        rma_ring_allreduce_get_remote_recv_buf_mr,
                        ar_handler);

        if (block_count * mlsl_datatype_get_size(dtype) > global_data.executor->max_order_waw_size)
            sched->add_barrier();

        e = entry_factory::make_write_entry(sched,
                                            &ar_handler->sync_flags[flag_idx_offset + idx],
                                            (atl_mr_t*)NULL, /* src_mr */
                                            sizeof(uint64_t),
                                            mlsl_dtype_internal_char,
                                            ar_handler->dst_peer,
                                            (atl_mr_t*)NULL, /* dst_mr */
                                            0 /* dst_buf_offset */);
        e->set_field_fn(mlsl_sched_entry_field_src_mr,
                        rma_ring_allreduce_get_sync_flags_mr,
                        ar_handler);
        e->set_field_fn(mlsl_sched_entry_field_dst_mr,
                        rma_ring_allreduce_get_remote_sync_flag_mr,
                        ar_handler);

        block_idx = (block_idx + comm_size - 1) % comm_size;
        entry_factory::make_wait_value_entry(sched, &(ar_handler->sync_flag),
                                             (flag_idx_offset + idx + 1), mlsl_condition_greater_or_equal);
    }

    return status;
}
