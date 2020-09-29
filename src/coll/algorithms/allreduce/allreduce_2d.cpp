// TODO: Rework to support non global creation
// #include "coll/algorithms/algorithms.hpp"
//#include "coll/algorithms/allreduce/allreduce_2d.hpp"
//#include "common/global/global.hpp"
//#include "sched/entry/factory/entry_factory.hpp"
//
//ccl_allreduce_2d_builder::ccl_allreduce_2d_builder(size_t base_size, bool switch_dims, ccl_comm* comm) {
//    size_t vector_size = comm->size();
//    std::vector<int> first_dim_colors(vector_size), second_dim_colors(vector_size);
//
//    for (size_t idx = 0; idx < vector_size; idx++) {
//        if (switch_dims) {
//            first_dim_colors[idx] = idx / base_size;
//            second_dim_colors[idx] = idx % base_size;
//        }
//        else {
//            first_dim_colors[idx] = idx % base_size;
//            second_dim_colors[idx] = idx / base_size;
//        }
//    }
//
//    first_dim_comm = std::shared_ptr<ccl_comm>(
//        ccl_comm::create_with_colors(first_dim_colors,
//                                     ccl::global_data::get().comm_ids.get(),
//                                     comm));
//    second_dim_comm = std::shared_ptr<ccl_comm>(
//        ccl_comm::create_with_colors(second_dim_colors,
//                                     ccl::global_data::get().comm_ids.get(),
//                                     comm));
//
//    if (comm->size() == 0) {
//        std::string first_dim_ranks, second_dim_ranks;
//        for (size_t idx = 0; idx < first_dim_comm->size(); idx++) {
//            first_dim_ranks +=
//                ((idx) ? " " : "") + std::to_string(first_dim_comm->get_global_rank(idx));
//        }
//        for (size_t idx = 0; idx < second_dim_comm->size(); idx++) {
//            second_dim_ranks +=
//                ((idx) ? " " : "") + std::to_string(second_dim_comm->get_global_rank(idx));
//        }
//        LOG_INFO("\n\nallreduce_2d",
//                 "\nbase_size: ",
//                 base_size,
//                 "\nswitch_dims: ",
//                 switch_dims,
//                 "\nfirst_dim_comm: size ",
//                 first_dim_comm->size(),
//                 ", ranks ",
//                 first_dim_ranks,
//                 "\nsecond_dim_comm: size ",
//                 second_dim_comm->size(),
//                 ", ranks ",
//                 second_dim_ranks,
//                 "\n");
//    }
//}
//
//ccl_allreduce_2d_builder::~ccl_allreduce_2d_builder() {
//    first_dim_comm.reset();
//    second_dim_comm.reset();
//}
//
//static void ccl_allreduce_2d_add_allreduce_allgather(ccl_sched* sched,
//                                                     ccl_buffer send_buf,
//                                                     ccl_buffer recv_buf,
//                                                     size_t count,
//                                                     const ccl_datatype& dtype,
//                                                     ccl::reduction op,
//                                                     ccl_comm* comm,
//                                                     size_t chunk_idx,
//                                                     size_t chunk_count) {
//    ccl_comm* first_dim_comm = ccl::global_data::get().allreduce_2d_builder->get_first_dim_comm();
//    ccl_comm* second_dim_comm = ccl::global_data::get().allreduce_2d_builder->get_second_dim_comm();
//
//    size_t dtype_size = dtype.size();
//    size_t main_chunk_size = count / chunk_count;
//    size_t last_chunk_size = main_chunk_size + count % chunk_count;
//    size_t cnt = (chunk_idx == (chunk_count - 1)) ? last_chunk_size : main_chunk_size;
//    ccl_buffer rbuf = recv_buf + chunk_idx * main_chunk_size * dtype_size;
//
//    size_t main_block_count = cnt / first_dim_comm->size();
//    size_t last_block_count = main_block_count + cnt % first_dim_comm->size();
//    size_t ar_count = (first_dim_comm->rank() == (first_dim_comm->size() - 1)) ? last_block_count
//                                                                               : main_block_count;
//
//    if (ar_count) {
//        /* TODO: add second level selection to distinguish high and low level algorithms */
//        ccl_buffer ar_buf = rbuf + first_dim_comm->rank() * main_block_count * dtype_size;
//        ccl_coll_build_starlike_allreduce(
//            sched, ar_buf, ar_buf, ar_count, dtype, op, second_dim_comm);
//        sched->add_barrier();
//    }
//
//    std::vector<size_t> ag_recv_counts(first_dim_comm->size(), main_block_count);
//    ag_recv_counts[first_dim_comm->size() - 1] = last_block_count;
//    ccl_coll_build_allgatherv(
//        sched, rbuf, ar_count, rbuf, ag_recv_counts.data(), dtype, first_dim_comm);
//}
//
//static void ccl_allreduce_2d_add_reduce_scatter_allreduce_allgather(ccl_sched* sched,
//                                                                    ccl_buffer send_buf,
//                                                                    ccl_buffer recv_buf,
//                                                                    size_t count,
//                                                                    const ccl_datatype& dtype,
//                                                                    ccl::reduction op,
//                                                                    ccl_comm* comm,
//                                                                    size_t chunk_idx,
//                                                                    size_t chunk_count) {
//    ccl_comm* first_dim_comm = ccl::global_data::get().allreduce_2d_builder->get_first_dim_comm();
//
//    size_t dtype_size = dtype.size();
//    size_t main_chunk_size = count / chunk_count;
//    size_t last_chunk_size = main_chunk_size + count % chunk_count;
//    size_t cnt = (chunk_idx == (chunk_count - 1)) ? last_chunk_size : main_chunk_size;
//    ccl_buffer sbuf = send_buf + chunk_idx * main_chunk_size * dtype_size;
//    ccl_buffer rbuf = recv_buf + chunk_idx * main_chunk_size * dtype_size;
//
//    ccl_coll_build_reduce_scatter(sched, sbuf, rbuf, cnt / comm->size(), dtype, op, first_dim_comm);
//    sched->add_barrier();
//
//    if (chunk_idx == (chunk_count - 1) || (chunk_count == 1)) {
//        ccl_allreduce_2d_add_allreduce_allgather(
//            sched, send_buf, recv_buf, count, dtype, op, comm, chunk_idx, chunk_count);
//    }
//    else {
//        entry_factory::make_entry<subsched_entry>(
//            sched,
//            chunk_idx,
//            [send_buf, recv_buf, count, &dtype, op, comm, chunk_idx, chunk_count](ccl_sched* s) {
//                ccl_allreduce_2d_add_allreduce_allgather(
//                    s, send_buf, recv_buf, count, dtype, op, comm, chunk_idx, chunk_count);
//            },
//            "AR_AG");
//
//        entry_factory::make_entry<subsched_entry>(
//            sched,
//            chunk_idx + 1,
//            [send_buf, recv_buf, count, &dtype, op, comm, chunk_idx, chunk_count](ccl_sched* s) {
//                ccl_allreduce_2d_add_reduce_scatter_allreduce_allgather(
//                    s, send_buf, recv_buf, count, dtype, op, comm, chunk_idx + 1, chunk_count);
//            },
//            "RS_AR_AG");
//    }
//}
//
//ccl_status_t ccl_allreduce_2d_builder::build(ccl_sched* sched,
//                                             ccl_buffer send_buf,
//                                             ccl_buffer recv_buf,
//                                             size_t count,
//                                             const ccl_datatype& dtype,
//                                             ccl::reduction op,
//                                             ccl_comm* comm) {
//    CCL_THROW_IF_NOT(sched && send_buf && recv_buf && count,
//                     "incorrect values, sched ",
//                     sched,
//                     ", send ",
//                     send_buf,
//                     " recv ",
//                     recv_buf);
//
//    ccl_status_t status = ccl_status_success;
//
//    size_t chunk_count = ccl::global_data::env().ar2d_chunk_count;
//
//    if (chunk_count == 0) {
//        LOG_ERROR("unexpected chunk_count");
//        chunk_count = 1;
//    }
//
//    LOG_DEBUG("build 2d allreduce, chunk_count ", chunk_count);
//
//    ccl_allreduce_2d_add_reduce_scatter_allreduce_allgather(
//        sched, send_buf, recv_buf, count, dtype, op, comm, 0 /* chunk_idx */, chunk_count);
//
//    return status;
//}
