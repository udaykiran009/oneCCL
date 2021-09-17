
/*
 *
 *  (C) 2001 by Argonne National Laboratory.
 *      See COPYRIGHT in top-level directory.
 */

#include "coll/algorithms/algorithms.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#include "sched/entry/coll/coll_entry_helper.hpp"
#include "sched/entry/factory/chunked_entry_factory.hpp"
#include "sched/entry/factory/entry_factory.hpp"

ccl::status ccl_coll_build_direct_allreduce(ccl_sched* sched,
                                            ccl_buffer send_buf,
                                            ccl_buffer recv_buf,
                                            size_t count,
                                            const ccl_datatype& dtype,
                                            ccl::reduction op,
                                            ccl_comm* comm) {
    LOG_DEBUG("build direct allreduce");

    entry_factory::create<allreduce_entry>(sched, send_buf, recv_buf, count, dtype, op, comm);
    return ccl::status::success;
}

ccl::status ccl_coll_build_rabenseifner_allreduce(ccl_sched* sched,
                                                  ccl_buffer send_buf,
                                                  ccl_buffer recv_buf,
                                                  size_t count,
                                                  const ccl_datatype& dtype,
                                                  ccl::reduction op,
                                                  ccl_comm* comm) {
    LOG_DEBUG("build Rabenseifner's allreduce");
    CCL_ASSERT(sched != nullptr, "empty sched");

    ccl::status status = ccl::status::success;
    int comm_size, rank, newrank, pof2, rem;
    int i, send_idx, recv_idx, last_idx, mask, newdst, dst, send_cnt, recv_cnt;
    int *cnts = NULL, *disps = NULL;
    size_t dtype_size = dtype.size();

    comm_size = comm->size();
    rank = comm->rank();
    ccl_buffer tmp_buf = sched->alloc_buffer({ count * dtype_size, send_buf });

    /* copy local data into recv_buf */

    if (send_buf != recv_buf) {
        entry_factory::create<copy_entry>(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    if (comm_size == 1)
        return status;

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2();

    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) { /* even */
            entry_factory::create<send_entry>(sched, recv_buf, count, dtype, rank + 1, comm);
            sched->add_barrier();

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = CCL_INVALID_PROC_IDX;
        }
        else { /* odd */
            entry_factory::create<recv_entry>(sched, tmp_buf, count, dtype, rank - 1, comm);
            sched->add_barrier();

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */
            entry_factory::create<reduce_local_entry>(
                sched, tmp_buf, count, recv_buf, nullptr, dtype, op);
            sched->add_barrier();

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != CCL_INVALID_PROC_IDX) {
        /* for the reduce-scatter, calculate the count that
         * each process receives and the displacement within
         * the buffer */
        cnts = static_cast<int*>(CCL_MALLOC(pof2 * sizeof(int), "counts"));
        disps = static_cast<int*>(CCL_MALLOC(pof2 * sizeof(int), "displacements"));

        /* the cnts calculations assume this */
        CCL_ASSERT(count >= static_cast<size_t>(pof2), "count ", count, ", pof2 ", pof2);

        for (i = 0; i < (pof2 - 1); i++)
            cnts[i] = count / pof2;
        cnts[pof2 - 1] = count - (count / pof2) * (pof2 - 1);

        disps[0] = 0;
        for (i = 1; i < pof2; i++)
            disps[i] = disps[i - 1] + cnts[i - 1];

        mask = 0x1;
        send_idx = recv_idx = 0;
        last_idx = pof2;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                send_idx = recv_idx + pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            }

            int can_use_recv_reduce = 0;
            ccl_buffer buf1 = (recv_buf + disps[send_idx] * dtype_size);
            ccl_buffer buf2 = (recv_buf + disps[recv_idx] * dtype_size);
            if (buf1 != buf2 &&
                ((buf1 + send_cnt * dtype_size <= buf2) || (buf2 + recv_cnt * dtype_size <= buf1)))
                can_use_recv_reduce = 1;

            CCL_ASSERT(can_use_recv_reduce);

            if (can_use_recv_reduce) {
                entry_factory::create<recv_reduce_entry>(sched,
                                                         (recv_buf + disps[recv_idx] * dtype_size),
                                                         recv_cnt,
                                                         nullptr,
                                                         dtype,
                                                         op,
                                                         dst,
                                                         ccl_buffer(),
                                                         comm);
                entry_factory::create<send_entry>(
                    sched, (recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst, comm);
                sched->add_barrier();
            }

            else {
                /* Send data from recv_buf. Recv into tmp_buf */
                entry_factory::create<recv_entry>(
                    sched, (tmp_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst, comm);
                /* sendrecv, no barrier here */
                entry_factory::create<send_entry>(
                    sched, (recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst, comm);
                sched->add_barrier();

                /* tmp_buf contains data received in this step.
                 * recv_buf contains data accumulated so far */

                /* This algorithm is used only for predefined ops
                 * and predefined ops are always commutative. */
                entry_factory::create<reduce_local_entry>(sched,
                                                          (tmp_buf + disps[recv_idx] * dtype_size),
                                                          recv_cnt,
                                                          (recv_buf + disps[recv_idx] * dtype_size),
                                                          nullptr,
                                                          dtype,
                                                          op);
                sched->add_barrier();
            }

            /* update send_idx for next iteration */
            send_idx = recv_idx;
            mask <<= 1;

            /* update last_idx, but not in last iteration
             * because the value is needed in the allgather
             * step below. */
            if (mask < pof2)
                last_idx = recv_idx + pof2 / mask;
        }

        /* now do the allgather */

        mask >>= 1;
        while (mask > 0) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            send_cnt = recv_cnt = 0;
            if (newrank < newdst) {
                /* update last_idx except on first iteration */
                if (mask != pof2 / 2)
                    last_idx = last_idx + pof2 / (mask * 2);

                recv_idx = send_idx + pof2 / (mask * 2);
                for (i = send_idx; i < recv_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < last_idx; i++)
                    recv_cnt += cnts[i];
            }
            else {
                recv_idx = send_idx - pof2 / (mask * 2);
                for (i = send_idx; i < last_idx; i++)
                    send_cnt += cnts[i];
                for (i = recv_idx; i < send_idx; i++)
                    recv_cnt += cnts[i];
            }

            entry_factory::create<recv_entry>(
                sched, (recv_buf + disps[recv_idx] * dtype_size), recv_cnt, dtype, dst, comm);
            /* sendrecv, no barrier here */
            entry_factory::create<send_entry>(
                sched, (recv_buf + disps[send_idx] * dtype_size), send_cnt, dtype, dst, comm);
            sched->add_barrier();

            if (newrank > newdst)
                send_idx = recv_idx;

            mask >>= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            entry_factory::create<send_entry>(sched, recv_buf, count, dtype, rank - 1, comm);
        }
        else { /* even */
            entry_factory::create<recv_entry>(sched, recv_buf, count, dtype, rank + 1, comm);
        }
    }

    CCL_FREE(cnts);
    CCL_FREE(disps);

    return status;
}

ccl::status ccl_coll_build_recursive_doubling_allreduce(ccl_sched* sched,
                                                        ccl_buffer send_buf,
                                                        ccl_buffer recv_buf,
                                                        size_t count,
                                                        const ccl_datatype& dtype,
                                                        ccl::reduction op,
                                                        ccl_comm* comm) {
    LOG_DEBUG("build recursive_doubling allreduce");

    ccl::status status = ccl::status::success;

    int pof2, rem, comm_size, rank;
    int newrank, mask, newdst, dst;

    comm_size = comm->size();
    rank = comm->rank();

    size_t dtype_size = dtype.size();

    ccl_buffer tmp_buf = sched->alloc_buffer({ count * dtype_size, send_buf });

    /* copy local data into recv_buf */
    if (send_buf != recv_buf) {
        entry_factory::create<copy_entry>(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    if (comm_size == 1)
        return status;

    /* get nearest power-of-two less than or equal to comm_size */
    pof2 = comm->pof2();
    rem = comm_size - pof2;

    /* In the non-power-of-two case, all even-numbered
     * processes of rank < 2*rem send their data to
     * (rank+1). These even-numbered processes no longer
     * participate in the algorithm until the very end. The
     * remaining processes form a nice power-of-two. */

    if (rank < 2 * rem) {
        if (rank % 2 == 0) { /* even */
            entry_factory::create<send_entry>(sched, recv_buf, count, dtype, rank + 1, comm);
            sched->add_barrier();

            /* temporarily set the rank to -1 so that this
             * process does not pariticipate in recursive
             * doubling */
            newrank = -1;
        }
        else { /* odd */
            entry_factory::create<recv_entry>(sched, tmp_buf, count, dtype, rank - 1, comm);
            sched->add_barrier();

            /* do the reduction on received data. since the
             * ordering is right, it doesn't matter whether
             * the operation is commutative or not. */

            entry_factory::create<reduce_local_entry>(
                sched, tmp_buf, count, recv_buf, nullptr, dtype, op);
            sched->add_barrier();

            /* change the rank */
            newrank = rank / 2;
        }
    }
    else /* rank >= 2*rem */
        newrank = rank - rem;

    if (newrank != -1) {
        mask = 0x1;
        while (mask < pof2) {
            newdst = newrank ^ mask;
            /* find real rank of dest */
            dst = (newdst < rem) ? newdst * 2 + 1 : newdst + rem;

            /* Send the most current data, which is in recv_buf. Recv
             * into tmp_buf */
            entry_factory::create<recv_entry>(sched, tmp_buf, count, dtype, dst, comm);
            /* sendrecv, no barrier here */
            entry_factory::create<send_entry>(sched, recv_buf, count, dtype, dst, comm);
            sched->add_barrier();

            /* tmp_buf contains data received in this step.
             * recv_buf contains data accumulated so far */
            entry_factory::create<reduce_local_entry>(
                sched, tmp_buf, count, recv_buf, nullptr, dtype, op);
            sched->add_barrier();

            mask <<= 1;
        }
    }

    /* In the non-power-of-two case, all odd-numbered
     * processes of rank < 2*rem send the result to
     * (rank-1), the ranks who didn't participate above. */
    if (rank < 2 * rem) {
        if (rank % 2) { /* odd */
            entry_factory::create<send_entry>(sched, recv_buf, count, dtype, rank - 1, comm);
        }
        else { /* even */
            entry_factory::create<recv_entry>(sched, recv_buf, count, dtype, rank + 1, comm);
        }
        sched->add_barrier();
    }

    return status;
}

ccl::status ccl_coll_build_starlike_allreduce(ccl_sched* sched,
                                              ccl_buffer send_buf,
                                              ccl_buffer recv_buf,
                                              size_t count,
                                              const ccl_datatype& dtype,
                                              ccl::reduction op,
                                              ccl_comm* comm) {
    LOG_DEBUG("build starlike allreduce");

    ccl::status status = ccl::status::success;
    int comm_size = comm->size();
    int this_rank = comm->rank();
    size_t* buffer_counts =
        static_cast<size_t*>(CCL_MALLOC(comm_size * sizeof(size_t), "buffer_count"));
    size_t* buffer_offsets =
        static_cast<size_t*>(CCL_MALLOC(comm_size * sizeof(size_t), "buffer_offsets"));
    size_t dtype_size = dtype.size();

    // copy local data into recv_buf
    if (send_buf != recv_buf) {
        entry_factory::create<copy_entry>(sched, send_buf, recv_buf, count, dtype);
        sched->add_barrier();
    }

    if (comm_size == 1)
        return status;

    // calculate counts and offsets for each rank
    size_t common_buffer_count = count / comm_size;
    for (int rank_idx = 0; rank_idx < comm_size; ++rank_idx) {
        buffer_counts[rank_idx] = common_buffer_count;
        buffer_offsets[rank_idx] = rank_idx * buffer_counts[rank_idx] * dtype_size;
    }
    buffer_counts[comm_size - 1] += count % comm_size;

    // recv_reduce buffer for current rank
    size_t this_rank_buf_size = buffer_counts[this_rank] * dtype_size;

    ccl_buffer tmp_buf;
    if (this_rank_buf_size) {
        tmp_buf = sched->alloc_buffer({ this_rank_buf_size * (comm_size - 1), send_buf });
    }

    size_t tmp_buf_recv_idx = 0;
    for (int rank_idx = 0; rank_idx < comm_size; ++rank_idx) {
        if (rank_idx != this_rank) {
            // send buffer to others
            entry_factory::make_chunked_send_entry(sched,
                                                   recv_buf + buffer_offsets[rank_idx],
                                                   buffer_counts[rank_idx],
                                                   dtype,
                                                   rank_idx,
                                                   comm);

            // recv part of buffer from others and perform reduce
            entry_factory::make_chunked_recv_reduce_entry(
                sched,
                recv_buf + buffer_offsets[this_rank],
                buffer_counts[this_rank],
                nullptr,
                dtype,
                op,
                rank_idx,
                tmp_buf + this_rank_buf_size * tmp_buf_recv_idx,
                comm);
            ++tmp_buf_recv_idx;
        }
    }

    sched->add_barrier();

    // allgatherv
    CCL_CALL(ccl_coll_build_naive_allgatherv(
        sched, recv_buf, buffer_counts[this_rank], recv_buf, buffer_counts, dtype, comm));

    CCL_FREE(buffer_counts);
    CCL_FREE(buffer_offsets);

    return status;
}

ccl::status ccl_coll_build_ring_allreduce(ccl_sched* sched,
                                          ccl_buffer send_buf,
                                          ccl_buffer recv_buf,
                                          size_t count,
                                          const ccl_datatype& dtype,
                                          ccl::reduction op,
                                          ccl_comm* comm) {
    int inplace = (send_buf == recv_buf) ? 1 : 0;
    LOG_DEBUG("build ring allreduce ", inplace ? "in-place" : "out-of-place");

    CCL_THROW_IF_NOT(sched && send_buf && recv_buf,
                     "incorrect values, sched ",
                     sched,
                     ", send ",
                     send_buf,
                     " recv ",
                     recv_buf);

    ccl::status status = ccl::status::success;

    ccl_coll_build_ring_reduce_scatter(sched, send_buf, recv_buf, count, dtype, op, comm);

    sched->add_barrier();

    int comm_size = comm->size();
    size_t main_block_count = count / comm_size;
    size_t last_block_count = main_block_count + count % comm_size;
    std::vector<size_t> recv_counts(comm_size, main_block_count);
    if (count % comm_size) {
        recv_counts[comm_size - 1] = last_block_count;
    }

    ccl_coll_build_ring_allgatherv(
        sched, recv_buf, recv_counts[comm->rank()], recv_buf, recv_counts.data(), dtype, comm);

    sched->add_barrier();

    return status;
}

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)

ccl::status ccl_coll_build_topo_ring_allreduce(ccl_sched* sched,
                                               ccl_buffer send_buf,
                                               ccl_buffer recv_buf,
                                               size_t count,
                                               const ccl_datatype& dtype,
                                               ccl::reduction op,
                                               ccl_comm* comm) {
    LOG_DEBUG("build topo_ring allreduce");

    ccl_coll_entry_param barrier_param{};
    barrier_param.ctype = ccl_coll_barrier;
    barrier_param.comm = comm;
    barrier_param.hint_algo.barrier = ccl_coll_barrier_ring;

    ccl_comm* pair_comm = comm->get_host_comm()->get_pair_comm().get()->get_ccl_comm().get();
    ccl_comm* even_comm = comm->get_host_comm()->get_even_comm().get()->get_ccl_comm().get();
    ccl_comm* node_comm = comm->get_host_comm()->get_node_comm().get()->get_ccl_comm().get();
    ccl_comm* r2r_comm = comm->get_host_comm()->get_r2r_comm().get()->get_ccl_comm().get();

    int comm_size = comm->size();
    int even_comm_size = even_comm->size();
    int node_comm_size = node_comm->size();

    bool is_single_node = (comm_size == node_comm_size);
    bool is_onesided = (comm_size == 2) && is_single_node;
    bool is_inplace = (send_buf == recv_buf);
    bool use_tmp_buf = (!is_onesided && !is_single_node) || (is_single_node && is_inplace);

    ccl_buffer tmp_buf{};
    if (use_tmp_buf) {
        ccl::alloc_param alloc_param(
            count * dtype.size(), ccl::buffer_type::ze, ccl::buffer_place::device);
        tmp_buf = sched->alloc_buffer(alloc_param);
    }

    std::vector<ze_handle_exchange_entry::mem_desc_t> in_buffers{
        { send_buf.get_ptr(), ccl::ze::ipc_mem_type::memory }, // 0
        { recv_buf.get_ptr(), ccl::ze::ipc_mem_type::memory }, // 1
    };

    if (use_tmp_buf) {
        in_buffers.push_back({ tmp_buf.get_ptr(), ccl::ze::ipc_mem_type::memory });
    }
    size_t peer_tmp_buf_idx = 2;

    int skip_rank = -1;
    if (ccl::global_data::env().enable_kernel_1s_ipc_wa) {
        skip_rank = ccl::global_data::env().kernel_1s_lead;
    }

    if (sched->coll_attr.to_cache) {
        sched->set_entry_exec_mode(ccl_sched_entry_exec_once);
        entry_factory::create<ze_handle_exchange_entry>(sched, node_comm, in_buffers, skip_rank);
        sched->add_barrier();
        sched->set_entry_exec_mode(ccl_sched_entry_exec_regular);

        // TODO: no need barrier for the first iteration where ze_handle_exchange_entry exists
        coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
    }
    else {
        entry_factory::create<ze_handle_exchange_entry>(sched, node_comm, in_buffers, skip_rank);
    }

    sched->add_barrier();

    if (is_onesided) {
        LOG_DEBUG("use onesided allreduce");

        if (comm->rank() == ccl::global_data::env().kernel_1s_lead) {
            entry_factory::create<ze_onesided_allreduce_entry>(
                sched, send_buf, recv_buf, count, dtype, op, comm);
            sched->add_barrier();
        }

        barrier_param.comm = comm;
        coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
        sched->add_barrier();
    }
    else if (is_single_node) {
        LOG_DEBUG("use single node allreduce");

        entry_factory::create<ze_ring_allreduce_entry>(
            sched, send_buf, recv_buf, tmp_buf, count, dtype, op, comm, peer_tmp_buf_idx);
        sched->add_barrier();
    }
    else if (node_comm_size % 2 == 0) {
        LOG_DEBUG("use multi level allreduce");

        if (pair_comm->rank() == ccl::global_data::env().kernel_1s_lead) {
            /* reduce stage */

            entry_factory::create<ze_reduce_entry>(
                sched, send_buf, recv_buf, count, dtype, op, pair_comm->rank(), pair_comm);
            sched->add_barrier();

            /* gpu allreduce stage */

            // TODO: replace by reduce_scatter
            entry_factory::create<ze_ring_allreduce_entry>(
                sched, recv_buf, recv_buf, tmp_buf, count, dtype, op, even_comm, peer_tmp_buf_idx);
            sched->add_barrier();

            /* host allreduce stage */

            size_t main_block_count = count / even_comm_size;
            size_t block_count = main_block_count;
            if (even_comm->rank() == even_comm_size - 1) {
                block_count += count % even_comm_size;
            }

            if (!is_single_node && block_count) {
                ccl::alloc_param host_buf_param(
                    block_count * dtype.size(), ccl::buffer_type::regular, ccl::buffer_place::host);
                ccl_buffer host_buf = sched->alloc_buffer(host_buf_param);
                size_t offset_bytes = main_block_count * even_comm->rank() * dtype.size();
                ccl_buffer partial_recv_buf = recv_buf + offset_bytes;
                entry_factory::create<copy_entry>(sched,
                                                  partial_recv_buf,
                                                  host_buf,
                                                  block_count,
                                                  dtype,
                                                  copy_attr(copy_direction::d2h));
                sched->add_barrier();

                ccl_coll_build_allreduce(
                    sched, host_buf, host_buf, block_count, dtype, op, r2r_comm);
                sched->add_barrier();

                entry_factory::create<copy_entry>(sched,
                                                  host_buf,
                                                  partial_recv_buf,
                                                  block_count,
                                                  dtype,
                                                  copy_attr(copy_direction::h2d));
                sched->add_barrier();
            }

            if (even_comm_size > 1) {
                /* gpu allgatherv stage */

                // We believe that here we do not need to wait for peers in even_comm
                // in case of one node launch, because ze_ring_allreduce_entry is a blocking entry
                // and ze_a2a_allgatherv_entry is implemented as a broadcast
                // (we do not read from the peer receive buffer).
                // Otherwise, if there are any changes here or instability,
                // then a barrier is needed here.
                // barrier_param.comm = even_comm;
                // coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
                // sched->add_barrier();

                std::vector<size_t> recv_counts(even_comm_size, main_block_count);
                recv_counts.at(even_comm->rank()) = block_count;
                entry_factory::create<ze_a2a_allgatherv_entry>(sched,
                                                               recv_buf,
                                                               block_count,
                                                               recv_buf,
                                                               recv_counts.data(),
                                                               dtype,
                                                               even_comm,
                                                               1 /* peer_recv_buf_idx */);
                sched->add_barrier();

                barrier_param.comm = even_comm;
                coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
                sched->add_barrier();
            }

            /* gpu copy to peer stage */

            int peer_copy_rank = (pair_comm->rank() + 1) % pair_comm->size();
            entry_factory::create<copy_entry>(
                sched,
                recv_buf,
                ccl_buffer(),
                count,
                dtype,
                copy_attr(
                    peer_copy_rank, 1 /* peer_recv_buf_idx */, copy_direction::d2d, pair_comm));
            sched->add_barrier();
        }

        // TODO: fix tag matching in barriers
        barrier_param.comm = pair_comm;
        coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
        sched->add_barrier();
    }
    else {
        CCL_THROW("unexpected comm size: ", comm_size, ", node comm size: ", node_comm_size);
    }

    return ccl::status::success;
}

ccl::status ccl_coll_build_topo_a2a_allreduce(ccl_sched* sched,
                                              ccl_buffer send_buf,
                                              ccl_buffer recv_buf,
                                              size_t count,
                                              const ccl_datatype& dtype,
                                              ccl::reduction op,
                                              ccl_comm* comm) {
    LOG_DEBUG("build topo_a2a allreduce");

    const std::vector<ze_handle_exchange_entry::mem_desc_t> in_buffers{
        { send_buf.get_ptr(), ccl::ze::ipc_mem_type::memory }, // 0
        { recv_buf.get_ptr(), ccl::ze::ipc_mem_type::memory }, // 1
    };

    ccl_coll_entry_param barrier_param{};
    barrier_param.ctype = ccl_coll_barrier;
    barrier_param.comm = comm;
    barrier_param.hint_algo.barrier = ccl_coll_barrier_ring;

    if (sched->coll_attr.to_cache) {
        sched->set_entry_exec_mode(ccl_sched_entry_exec_once);
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers);
        sched->add_barrier();
        sched->set_entry_exec_mode(ccl_sched_entry_exec_regular);

        // TODO: no need barrier for the first iteration where ze_handle_exchange_entry exists
        coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
    }
    else {
        entry_factory::create<ze_handle_exchange_entry>(sched, comm, in_buffers);
    }

    sched->add_barrier();

    size_t segment_count = count / comm->size();
    if ((segment_count > 0) || (segment_count == 0 && static_cast<size_t>(comm->rank()) < count)) {
        entry_factory::create<ze_a2a_allreduce_entry>(
            sched, send_buf, recv_buf, count, dtype, op, comm);
        sched->add_barrier();
    }

    coll_entry_helper::add_coll_entry<ccl_coll_barrier>(sched, barrier_param);
    sched->add_barrier();

    return ccl::status::success;
}

#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
