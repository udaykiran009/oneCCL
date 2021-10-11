#include "atl/util/pm/pmi_resizable_rt/pmi_resizable/kvs/users_kvs.h"
#include "exec/exec.hpp"
#include "common/comm/comm.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#include "common/global/global.hpp"
#include "sched/sched.hpp"
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/kvs.hpp"

void ccl_comm::allocate_resources() {
    if (ccl::global_data::env().enable_unordered_coll) {
        unordered_coll_manager =
            std::unique_ptr<ccl_unordered_coll_manager>(new ccl_unordered_coll_manager(*this));
    }

    auto& env_object = ccl::global_data::env();

    allreduce_2d_builder = std::unique_ptr<ccl_allreduce_2d_builder>(new ccl_allreduce_2d_builder(
        (env_object.allreduce_2d_base_size != CCL_ENV_SIZET_NOT_SPECIFIED)
            ? env_object.allreduce_2d_base_size
            : ccl::global_data::get().executor->get_local_proc_count(),
        env_object.allreduce_2d_switch_dims,
        this));

    env_object.print(m_rank);
}

ccl_comm::ccl_comm(int rank,
                   int size,
                   ccl_comm_id_storage::comm_id&& id,
                   std::shared_ptr<atl_base_comm> atl,
                   bool share_resources,
                   ccl::host_communicator* host_comm)
        : ccl_comm(rank,
                   size,
                   std::move(id),
                   ccl_rank2rank_map{},
                   atl,
                   share_resources,
                   host_comm) {}

ccl_comm::ccl_comm(int rank,
                   int size,
                   ccl_comm_id_storage::comm_id&& id,
                   ccl_rank2rank_map&& rank_map,
                   std::shared_ptr<atl_base_comm> atl,
                   bool share_resources,
                   ccl::host_communicator* host_comm)
        : atl(atl),
          m_id(std::move(id)),
          m_local2global_map(std::move(rank_map)),
          m_dtree(size, rank),
          thread_number(1),
          on_process_ranks_number(1),
          host_comm(host_comm) {
    reset(rank, size);

    if (!share_resources) {
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

ccl_comm::ccl_comm(const std::vector<int>& local_ranks,
                   int comm_size,
                   std::shared_ptr<ccl::kvs_interface> kvs_instance,
                   ccl_comm_id_storage::comm_id&& id,
                   bool share_resources,
                   ccl::host_communicator* host_comm)
        : m_id(std::move(id)),
          m_local2global_map(),
          m_dtree(local_ranks.size(), comm_size),
          host_comm(host_comm) {
    std::shared_ptr<ikvs_wrapper> kvs_wrapper(new users_kvs(kvs_instance));

    atl = atl_comm_manager::create_comm(comm_size, local_ranks, kvs_wrapper);

    thread_number = atl->get_threads_per_process();
    on_process_ranks_number = atl->get_ranks_per_process();

    reset(atl->get_rank(), atl->get_size());

    if (!share_resources) {
        allocate_resources();
    }
}

ccl_comm* ccl_comm::create_with_color(int color,
                                      ccl_comm_id_storage* comm_ids,
                                      bool share_resources) const {
    std::shared_ptr<atl_base_comm> atl_comm = atl->comm_split(color);
    ccl_comm* comm = new ccl_comm(atl_comm->get_rank(),
                                  atl_comm->get_size(),
                                  comm_ids->acquire(),
                                  atl_comm->get_rank2rank_map(),
                                  atl_comm,
                                  share_resources);
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
    return std::make_shared<ccl_comm>(m_rank,
                                      m_size,
                                      std::move(id),
                                      std::move(rank_map),
                                      atl,
                                      true /*share_resources*/,
                                      get_host_comm());
}
//TODO: will fix it after OFI refactoring
int ccl_comm::get_global_rank(int rank, bool only_global) const {
    if (m_local2global_map.empty() || !only_global) {
        // global comm and its copies do not have entries in the map
        return rank;
    }

    CCL_THROW_IF_NOT((int)m_local2global_map.size() > rank,
                     "no rank ",
                     rank,
                     " was found in comm ",
                     this,
                     ", id ",
                     m_id.value());
    int global_rank = m_local2global_map[rank];
    LOG_DEBUG(
        "comm ", this, ", id ", m_id.value(), ", map rank ", rank, " to global ", global_rank);
    return global_rank;
}

int ccl_comm::get_rank_from_global(int global_rank) const {
    if (m_local2global_map.empty()) {
        // global comm and its copies do not have entries in the map
        return global_rank;
    }

    int rank = ccl_comm::invalid_rank;

    for (size_t i = 0; i < m_local2global_map.size(); ++i) {
        if (m_local2global_map[i] == global_rank) {
            rank = static_cast<int>(i);
            break;
        }
    }

    CCL_THROW_IF_NOT(rank != ccl_comm::invalid_rank,
                     "no rank ",
                     global_rank,
                     " was found in comm ",
                     this,
                     ", id ",
                     m_id.value());

    return rank;
}
