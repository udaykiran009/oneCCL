#include "coll/coll_algorithms.hpp"
#include "coll/sparse_types.hpp"
#include "sched/entry_factory.hpp"

mlsl_status_t sparse_before_recv(const void* ctx)
{
    /* if our neighbour sent us data more than we ever had before
    we have to alloc more memory for incoming msg*/
    mlsl_sparse_allreduce_handler *sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    if (*sa_handler->recv_icount > sa_handler->recv_buff_size)
    {
        *sa_handler->recv_buf = MLSL_REALLOC(*sa_handler->recv_buf, sa_handler->recv_buff_size, *sa_handler->recv_icount, 64, "recv_buf");
        sa_handler->recv_buff_size = *sa_handler->recv_icount;
    }
    return mlsl_status_success;
}

mlsl_status_t sparse_after_recv(const void* ctx)
{
    mlsl_sparse_allreduce_handler *sa_handler = (mlsl_sparse_allreduce_handler*)ctx;

    /* having received the msg we should prepare it for further send operation to the next neighbour */
    sa_handler->send_count[0] = *sa_handler->recv_icount / (sa_handler->itype_size + sa_handler->vtype_size * sa_handler->val_dim_cnt);
    sa_handler->send_count[1] = sa_handler->send_count[0] * sa_handler->val_dim_cnt;

    /* TO BE DELETED:
    int *idx = (int*)(*sa_handler->recv_buf);
    float *val = (float*)((char*)(*sa_handler->recv_buf) + sa_handler->itype_size * sa_handler->send_count[0]);
    for (unsigned int i = 0; i < sa_handler->send_count[0]; i++)
    {
        char str[128];
        sprintf(str, "Received buf[%d] = (i: %i, v:", i, idx[i]);
        for (unsigned int j = 0; j < sa_handler->val_dim_cnt; j++)
            sprintf(str, "%s %f", str, val[i * sa_handler->val_dim_cnt + j]);
        printf("%s)\n", str);fflush(stdout);
    }*/

    return mlsl_status_success;
}

mlsl_status_t sparse_reduce(const void* ctx)
{
    mlsl_sparse_allreduce_handler *sa_handler = (mlsl_sparse_allreduce_handler*)ctx;

    int *snd_i = (int*)(sa_handler->dst_buf);
    float *snd_v = (float*)((char*)(sa_handler->dst_buf) + sa_handler->itype_size * sa_handler->dst_count[0]);

    /* copy data from recv_buf so that it would be easier to identify unique indices */
    size_t idx_size = sa_handler->itype_size * sa_handler->send_count[0];
    std::vector<int> rcv_i(sa_handler->send_count[0]);
    memcpy(rcv_i.data(), (int*)(*sa_handler->recv_buf), idx_size);
    float *rcv_v = (float*)((char*)(*sa_handler->recv_buf) + idx_size);

    /* Look at received indices and the ones we already have. Check if there are equal
    ones, then the values could be reduced right away. The indices left will be copied
    along with correspoinding values*/
    size_t unique_indices = 0;
    for (size_t num = 0; num < sa_handler->send_count[0]; num++)
    {
        idx_offset_map::iterator key = sa_handler->iv_map->find(rcv_i[num]);
        if (key != sa_handler->iv_map->end())
        {
            for (size_t k = 0; k < sa_handler->val_dim_cnt; k++)
            {
                snd_v[key->second + k] += rcv_v[num * sa_handler->val_dim_cnt + k];
                /* we're done with this idx, it's not unique */
                rcv_i[num] = -1;
            }
        }
        else
        {
            /* we'll run through these unique indices later */
            unique_indices++;
        }
    }

    /* Were there any unique indices? */
    if (unique_indices > 0)
    {
        /* prepare buf for combined data */
        size_t merge_idx_len = sa_handler->iv_map->size() + unique_indices;

        std::vector<int> buf_i(merge_idx_len);
        std::vector<float> buf_v(merge_idx_len * sa_handler->val_dim_cnt);

        /* copy what we already have reduced*/
        memcpy(buf_i.data(), snd_i, sa_handler->itype_size * sa_handler->dst_count[0]);
        memcpy(buf_v.data(), snd_v, sa_handler->vtype_size * sa_handler->dst_count[1]);

        size_t idx_offset = 0;
        for (size_t num = 0; num < sa_handler->send_count[0]; num++)
        {
            if (rcv_i[num] > -1)
            {
                buf_i[sa_handler->dst_count[0] + idx_offset] = rcv_i[num];

                for (size_t k = 0; k < sa_handler->val_dim_cnt; k++)
                {
                    buf_v[sa_handler->dst_count[1] + idx_offset * sa_handler->val_dim_cnt + k] = rcv_v[num * sa_handler->val_dim_cnt + k];
                }

                /* upd the map */
                sa_handler->iv_map->emplace(rcv_i[num], sa_handler->dst_count[1] + idx_offset * sa_handler->val_dim_cnt);
                idx_offset++;
            }
        }

        /* we definitely have to increase the size of dst buffer because
        of the unique indices that came from our neighbour */
        size_t new_dst_size = merge_idx_len * (sa_handler->vtype_size * sa_handler->val_dim_cnt + sa_handler->itype_size);
        void* ptr = MLSL_REALLOC(sa_handler->dst_buf, sa_handler->dst_count[0] * sa_handler->itype_size + sa_handler->dst_count[1] * sa_handler->vtype_size, new_dst_size, 64, "dst");
        if (ptr != sa_handler->dst_buf)
        {
            for (auto& x: *sa_handler->buf_list)
            {
                if (x.ptr == sa_handler->dst_buf)
                {
                    x.ptr = ptr;
                    x.size = new_dst_size;
                    break;
                }
            }
            sa_handler->dst_buf = ptr;
        }

        memcpy((int*)(sa_handler->dst_buf), buf_i.data(), sa_handler->itype_size * merge_idx_len);
        memcpy((float*)((char*)(sa_handler->dst_buf) + sa_handler->itype_size * merge_idx_len), buf_v.data(), sa_handler->vtype_size * merge_idx_len * sa_handler->val_dim_cnt);

        sa_handler->dst_count[0] = merge_idx_len;
        sa_handler->dst_count[1] = merge_idx_len * sa_handler->val_dim_cnt;

    } // if unique_indices > 0

    if (*sa_handler->recv_icount > sa_handler->send_buff_size)
    {
        void* ptr = MLSL_REALLOC(sa_handler->send_tmp_buf, sa_handler->send_buff_size, *sa_handler->recv_icount, 64, "send_tmp_buf");
        if (ptr != sa_handler->send_tmp_buf)
        {
            for (auto& x: *sa_handler->buf_list)
            {
                if (x.ptr == sa_handler->send_tmp_buf)
                {
                    x.ptr = ptr;
                    x.size = *sa_handler->recv_icount;
                    break;
                }
            }
            sa_handler->send_tmp_buf = ptr;
        }
        sa_handler->send_buff_size = *sa_handler->recv_icount;

    }
    memcpy(sa_handler->send_tmp_buf, *sa_handler->recv_buf, *sa_handler->recv_icount);

    return mlsl_status_success;
}

mlsl_status_t sparse_prepare_result(const void* ctx)
{
    mlsl_sparse_allreduce_handler *sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    *sa_handler->recv_icount = sa_handler->iv_map->size();
    *sa_handler->recv_vcount = *sa_handler->recv_icount * sa_handler->val_dim_cnt;
    size_t total_size = sa_handler->itype_size * (*sa_handler->recv_icount) + sa_handler->vtype_size * (*sa_handler->recv_vcount);

    if (sa_handler->recv_buff_size < total_size)
    { 
        *sa_handler->recv_buf = MLSL_REALLOC(*sa_handler->recv_buf, 0ul, total_size, 64, "recv_buf");
    }

    sa_handler->iv_map->clear();
    delete sa_handler->iv_map;

    memcpy(*sa_handler->recv_buf, sa_handler->dst_buf, total_size);
    *sa_handler->recv_vbuf = ((char*)(*sa_handler->recv_buf) + sa_handler->itype_size * (*sa_handler->recv_icount));
    return mlsl_status_success;
}

mlsl_status_t sparse_get_send_cnt(const void* ctx, void* field_ptr)
{
    mlsl_sparse_allreduce_handler* sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    size_t* cnt_ptr = (size_t*)field_ptr;
    *cnt_ptr = sa_handler->send_count[0] * (sa_handler->itype_size + sa_handler->val_dim_cnt * sa_handler->vtype_size);
    return mlsl_status_success;
}

mlsl_status_t sparse_get_send_buf(const void* ctx, void* field_ptr)
{
    mlsl_sparse_allreduce_handler* sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    void** buf_ptr = (void**)field_ptr;
    *buf_ptr = sa_handler->send_tmp_buf;
    return mlsl_status_success;
}

mlsl_status_t sparse_get_recv_cnt(const void* ctx, void* field_ptr)
{
    mlsl_sparse_allreduce_handler* sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    size_t* cnt_ptr = (size_t*)field_ptr;
    *cnt_ptr = *sa_handler->recv_icount;
    return mlsl_status_success;
}

mlsl_status_t sparse_get_recv_buf(const void* ctx, void* field_ptr)
{
    mlsl_sparse_allreduce_handler* sa_handler = (mlsl_sparse_allreduce_handler*)ctx;
    void** buf_ptr = (void**)field_ptr;
    *buf_ptr = *sa_handler->recv_buf;
    return mlsl_status_success;
}

mlsl_status_t mlsl_coll_build_sparse_allreduce_basic(mlsl_sched *sched, const void *send_ind_buf, size_t send_ind_count,
                                                     const void *send_val_buf, size_t send_val_count, void **recv_ind_buf,
                                                     size_t *recv_ind_count, void **recv_val_buf, size_t *recv_val_count,
                                                     mlsl_datatype_internal_t index_dtype, mlsl_datatype_internal_t value_dtype, mlsl_reduction_t op)
{
    LOG_DEBUG("build sparse allreduce");

    mlsl_status_t status = mlsl_status_success;
    std::shared_ptr<sched_entry> e;

    mlsl_comm *comm = sched->coll_param.comm;
    size_t comm_size = comm->size();
    size_t rank = comm->rank();

    /* get data type sizes */
    size_t vtype_size = mlsl_datatype_get_size(value_dtype);
    size_t itype_size = mlsl_datatype_get_size(index_dtype);
    /* TODO: get rid of hard-coded int and float indices and values definition */

    /* get value dimension */
    size_t val_dim_cnt = send_val_count / send_ind_count;

    /* the accumulated result will be kept here */
    void* dst = nullptr;

    /* buffers for in_data */
    int* src_i = nullptr;
    float* src_v = nullptr;

    if (send_ind_buf != *recv_ind_buf)
    {
        src_i = (int*)send_ind_buf;
        src_v = (float*)send_val_buf;
    }
    else
    {
        src_i = (int*)*recv_ind_buf;
        src_v = (float*)*recv_val_buf;
    }

    /* fill in the <index:value_offset> map */
    idx_offset_map *iv_map = new idx_offset_map();
    for (size_t i = 0; i < send_ind_count; i++)
    {
        idx_offset_map::iterator key = iv_map->find(src_i[i]);
        if (key == iv_map->end())
        {
            /* save index and starting addr of values set according to that index */
            iv_map->emplace(src_i[i], i * val_dim_cnt);
        }
        else
        {
            /* reduce values from duplicate indices */
            /* TODO: use operation instead of hard-coded sum */
            for (size_t k = 0; k < val_dim_cnt; k++)
            {
                src_v[key->second + k] += src_v[i * val_dim_cnt + k];
            }
        }
    }

    /* create buffer w/o duplicates */
    size_t iv_map_cnt = iv_map->size();
    size_t nodup_size = iv_map_cnt * (itype_size + val_dim_cnt * vtype_size);
    dst = sched->alloc_buffer(nodup_size);

    int* dst_i = (int*)dst;
    float* dst_v = (float*)((char*)dst + itype_size * iv_map_cnt);

    size_t idx_offset = 0;
    size_t val_offset = 0;

    /* update value offsets in the map, because we copy data to dst buffer from source buffer */
    for (auto& it : *iv_map)
    {
        dst_i[idx_offset] = it.first;
        val_offset = idx_offset * val_dim_cnt;
        memcpy(dst_v + val_offset, src_v + it.second, vtype_size * val_dim_cnt);
        it.second = val_offset;
        idx_offset++;
    }

    /* copy data to the temp send buffer for send operation */
    void *send_tmp_buf = nullptr;
    send_tmp_buf = sched->alloc_buffer(nodup_size);
    memcpy(send_tmp_buf, dst, nodup_size);

    /* send from left to right (ring)*/

    /* receive from the left neighbour */
    size_t recv_from = (rank - 1 + comm_size) % comm_size;

    /* send to the right neighbour */
    size_t send_to = (rank + 1) % comm_size;

    /* create handler for sched function callbacks */
    mlsl_sparse_allreduce_handler *sa_handler;
    sa_handler = static_cast<mlsl_sparse_allreduce_handler*>(sched->alloc_buffer(sizeof(mlsl_sparse_allreduce_handler)));

    /* _count variables needed for sending/receiving */
    sa_handler->send_count[0] = iv_map_cnt; /* index count */
    sa_handler->send_count[1] = iv_map_cnt * val_dim_cnt; /* value count */
    memcpy(&sa_handler->dst_count, &sa_handler->send_count, sizeof(size_t) * 2);
    sa_handler->recv_buff_size = nodup_size;
    sa_handler->send_buff_size = nodup_size;
    sa_handler->val_dim_cnt = val_dim_cnt;
    sa_handler->itype_size = itype_size;
    sa_handler->vtype_size = vtype_size;
    sa_handler->dst_buf = dst;
    sa_handler->send_tmp_buf = send_tmp_buf;
    sa_handler->recv_buf = recv_ind_buf;
    sa_handler->recv_vbuf = recv_val_buf;
    sa_handler->recv_icount = recv_ind_count;
    sa_handler->recv_vcount = recv_val_count;
    sa_handler->iv_map = iv_map;
    sa_handler->buf_list = &(sched->memory.buf_list);

    for (size_t i = 0; i < comm_size - 1; i++)
    {
        /* send local data to the right neighbour */
        e = entry_factory::make_send_entry(sched, NULL, 0, mlsl_dtype_internal_char, send_to);
        e->set_field_fn(mlsl_sched_entry_field_cnt, sparse_get_send_cnt, sa_handler);
        e->set_field_fn(mlsl_sched_entry_field_buf, sparse_get_send_buf, sa_handler);
        sched->add_barrier();

        /* has the left neighbour sent anything? */
        entry_factory::make_probe_entry(sched, recv_from, sa_handler->recv_icount);
        sched->add_barrier();

        /* receive data from the left neighbour */
        entry_factory::make_function_entry(sched, sparse_before_recv, sa_handler);
        sched->add_barrier();
        e = entry_factory::make_recv_entry(sched, NULL, 0, mlsl_dtype_internal_char, recv_from);
        e->set_field_fn(mlsl_sched_entry_field_buf, sparse_get_recv_buf, sa_handler);
        e->set_field_fn(mlsl_sched_entry_field_cnt, sparse_get_recv_cnt, sa_handler);
        sched->add_barrier();
        entry_factory::make_function_entry(sched, sparse_after_recv, sa_handler);
        sched->add_barrier();

        /* reduce data */
        entry_factory::make_function_entry(sched, sparse_reduce, sa_handler);
        sched->add_barrier();
    }

    /* copy all reduced data to recv_buf */
    entry_factory::make_function_entry(sched, sparse_prepare_result, sa_handler);
    sched->add_barrier();

    return status;
}
