#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/users_kvs.h"
#include "exec/exec.hpp"
#include "common/comm/comm.hpp"
#include "common/global/global.hpp"
#include "sched/sched.hpp"
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_kvs.hpp"

void ccl_comm::allocate_resources()
{
    if (ccl::global_data::env().enable_unordered_coll) {
        unordered_coll_manager =
            std::unique_ptr<ccl_unordered_coll_manager>(new ccl_unordered_coll_manager(*this));
    }

    auto& env_object = ccl::global_data::env();

    allreduce_2d_builder = std::unique_ptr<ccl_allreduce_2d_builder>(
      new ccl_allreduce_2d_builder(
          (env_object.allreduce_2d_base_size != CCL_ENV_SIZET_NOT_SPECIFIED)
              ? env_object.allreduce_2d_base_size
               : ccl::global_data::get().executor->get_local_proc_count(),
           env_object.allreduce_2d_switch_dims,
           this));

    if (m_rank == 0)
        env_object.print();
}

ccl_comm::ccl_comm(size_t rank,
                   size_t size,
                   ccl_comm_id_storage::comm_id&& id,
                   std::shared_ptr<atl_wrapper> atl,
                   bool share_resources)
        : ccl_comm(rank, size, std::move(id), ccl_rank2rank_map{}, atl, share_resources) {}

ccl_comm::ccl_comm(size_t rank,
                   size_t size,
                   ccl_comm_id_storage::comm_id&& id,
                   ccl_rank2rank_map&& rank_map,
                   std::shared_ptr<atl_wrapper> atl,
                   bool share_resources)
        : atl(atl),
          m_id(std::move(id)),
          m_local2global_map(std::move(rank_map)),
          m_dtree(size, rank),
          thread_number(1),
          on_process_ranks_number(1) {
    reset(rank, size);

    if (!share_resources)
    {
        allocate_resources();
    }
}

//TODO non-implemented
//TODO rude simulation of multi-thread barrier
static std::atomic<size_t> thread_counter{};
static std::atomic<size_t> thread_ranks_counter{};
void ccl_comm::ccl_comm_reset_thread_barrier() {
    // recharge counters again
    thread_counter.store(0);
    thread_ranks_counter.store(0);
}

ccl_comm::ccl_comm(const std::vector<size_t>& local_thread_device_ranks,
                   size_t cluster_devices_count,
                   std::shared_ptr<ccl::kvs_interface> kvs_instance,
                   ccl_comm_id_storage::comm_id&& id,
                   bool share_resources)
        : m_id(std::move(id)),
          m_local2global_map(),
          m_dtree(local_thread_device_ranks.size(), cluster_devices_count) {

    std::shared_ptr<ikvs_wrapper> kvs_wrapper(new users_kvs(kvs_instance));

    atl = std::shared_ptr<atl_wrapper>(
        new atl_wrapper(cluster_devices_count, local_thread_device_ranks, kvs_wrapper));

    thread_number = atl->get_threads_count();
    on_process_ranks_number = atl->get_devices_per_rank_count();

    reset(atl->get_rank(), atl->get_size());

    if (!share_resources)
    {
        allocate_resources();
    }
}

static ccl_status_t ccl_comm_exchange_colors(std::vector<int>& colors) {
    throw ccl::exception("ccl_comm_exchange_colors not implemented yet");

    // const size_t exchange_count = 1;
    // std::vector<size_t> recv_counts(colors.size(), exchange_count);
    // ccl_coll_attr_t coll_attr{};
    // coll_attr.to_cache = false;
    // ccl_request_t request;

    // ccl_status_t status;

    // CCL_CALL(ccl_allgatherv(colors.data(), exchange_count,
    //                         colors.data(), recv_counts.data(),
    //                         ccl_dtype_int, &coll_attr,
    //                         nullptr, /* comm */
    //                         nullptr, /* stream */
    //                         &request));

    // CCL_CALL(ccl_wait(request));

    // return status;
}

ccl_comm* ccl_comm::create_with_color(int color,
                                      ccl_comm_id_storage* comm_ids,
                                      const ccl_comm* parent_comm) {
    throw ccl::exception("unimplemented yet");

    if (ccl::global_data::env().atl_transport == ccl_atl_mpi) {
        throw ccl::exception(
            "MPI transport doesn't support creation of communicator with color yet");
    }

    ccl_status_t status = ccl_status_success;

    std::vector<int> colors(parent_comm->size());
    colors[parent_comm->rank()] = color;
    status = ccl_comm_exchange_colors(colors);
    if (status != ccl_status_success) {
        throw ccl::exception("failed to exchange colors during comm creation");
    }

    return create_with_colors(colors, comm_ids, parent_comm);
}

ccl_comm* ccl_comm::create_with_colors(const std::vector<int>& colors,
                                       ccl_comm_id_storage* comm_ids,
                                       const ccl_comm* parent_comm,
                                       bool share_resources) {
    ccl_rank2rank_map rank_map;
    size_t new_comm_size = 0;
    size_t new_comm_rank = 0;
    int color = colors[parent_comm->rank()];

    for (size_t i = 0; i < parent_comm->size(); ++i) {
        if (colors[i] == color) {
            LOG_DEBUG("map local rank ", new_comm_size, " to global ", i);
            rank_map.emplace_back(i);
            ++new_comm_size;
            if (i < parent_comm->rank()) {
                ++new_comm_rank;
            }
        }
    }

    if (new_comm_size == 0) {
        throw ccl::exception(std::string("no colors matched to ") + std::to_string(color) +
                             " seems to be exchange issue");
    }

    if (new_comm_size == parent_comm->size()) {
        // exact copy of the global communicator, use empty map
        rank_map.clear();
    }

    ccl_comm* comm = new ccl_comm(
        new_comm_rank, new_comm_size, comm_ids->acquire(), std::move(rank_map), parent_comm->atl, share_resources);

    LOG_DEBUG("new comm: color ",
              color,
              ", rank ",
              comm->rank(),
              ", size ",
              comm->size(),
              ", comm_id ",
              comm->id());

    return comm;
}

std::shared_ptr<ccl_comm> ccl_comm::clone_with_new_id(ccl_comm_id_storage::comm_id&& id) {
    ccl_rank2rank_map rank_map{ m_local2global_map };
    return std::make_shared<ccl_comm>(m_rank, m_size, std::move(id), std::move(rank_map), atl, true /*share_resources*/);
}

size_t ccl_comm::get_global_rank(size_t rank) const {
    if (m_local2global_map.empty()) {
        // global comm and its copies do not have entries in the map
        return rank;
    }

    CCL_THROW_IF_NOT(m_local2global_map.size() > rank,
                     "no rank ",
                     rank,
                     " was found in comm ",
                     this,
                     ", id ",
                     m_id.value());
    size_t global_rank = m_local2global_map[rank];
    LOG_DEBUG(
        "comm , ", this, " id ", m_id.value(), ", map rank ", rank, " to global ", global_rank);
    return global_rank;
}
