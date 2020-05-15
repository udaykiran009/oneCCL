#include "ccl.h"
#include "common/comm/comm.hpp"
#include "common/env/env.hpp"
#include "sched/sched.hpp"

ccl_comm::ccl_comm(size_t rank,
                   size_t size,
                   ccl_comm_id_storage::comm_id&& id) :
    ccl_comm(rank, size, std::move(id), ccl_rank2rank_map{})
{}

ccl_comm::ccl_comm(size_t rank,
                   size_t size,
                   ccl_comm_id_storage::comm_id&& id,
                   ccl_rank2rank_map&& rank_map) :
    m_id(std::move(id)),
    m_local2global_map(std::move(rank_map)),
    m_dtree(size, rank)
{
    reset(rank, size);
}

static ccl_status_t ccl_comm_exchange_colors(std::vector<int>& colors)
{
    const size_t exchange_count = 1;
    std::vector<size_t> recv_counts(colors.size(), exchange_count);
    ccl_coll_attr_t coll_attr{};
    coll_attr.to_cache = false;
    ccl_request_t request;

    ccl_status_t status;

    CCL_CALL(ccl_allgatherv(colors.data(), exchange_count,
                            colors.data(), recv_counts.data(),
                            ccl_dtype_int, &coll_attr,
                            nullptr, /* comm */
                            nullptr, /* stream */
                            &request));

    CCL_CALL(ccl_wait(request));

    return status;
}

ccl_comm* ccl_comm::create_with_color(int color,
                                      ccl_comm_id_storage* comm_ids,
                                      const ccl_comm* global_comm)
{
    if (env_data.atl_transport == ccl_atl_mpi)
    {
        throw ccl::ccl_error("MPI transport doesn't support creation of communicator with color yet");
    }
    
    ccl_status_t status = ccl_status_success;

    std::vector<int> colors(global_comm->size());
    colors[global_comm->rank()] = color;
    status = ccl_comm_exchange_colors(colors);
    if (status != ccl_status_success)
    {
        throw ccl::ccl_error("failed to exchange colors during comm creation");
    }

    return create_with_colors(colors, comm_ids, global_comm);
}

ccl_comm* ccl_comm::create_with_colors(const std::vector<int>& colors,
                                       ccl_comm_id_storage* comm_ids,
                                       const ccl_comm* global_comm)
{
    ccl_rank2rank_map rank_map;
    size_t new_comm_size = 0;
    size_t new_comm_rank = 0;
    int color = colors[global_comm->rank()];

    for (size_t i = 0; i < global_comm->size(); ++i)
    {
        if (colors[i] == color)
        {
            LOG_DEBUG("map local rank ", new_comm_size, " to global ", i);
            rank_map.emplace_back(i);
            ++new_comm_size;
            if (i < global_comm->rank())
            {
                ++new_comm_rank;
            }
        }
    }

    if (new_comm_size == 0)
    {
        throw ccl::ccl_error(
            std::string("no colors matched to ") + std::to_string(color) + " seems to be exchange issue");
    }

    if (new_comm_size == global_comm->size())
    {
        // exact copy of the global communicator, use empty map
        rank_map.clear();
    }

    ccl_comm* comm = new ccl_comm(new_comm_rank, new_comm_size, comm_ids->acquire(),
                                  std::move(rank_map));

    LOG_DEBUG("new comm: color ", color,
              ", rank ", comm->rank(),
              ", size ", comm->size(),
              ", comm_id ", comm->id());

    return comm;
}

std::shared_ptr<ccl_comm> ccl_comm::clone_with_new_id(ccl_comm_id_storage::comm_id&& id)
{
    ccl_rank2rank_map rank_map{m_local2global_map};
    return std::make_shared<ccl_comm>(m_rank, m_size, std::move(id), std::move(rank_map));
}

size_t ccl_comm::get_global_rank(size_t rank) const
{
    if (m_local2global_map.empty())
    {
        // global comm and its copies do not have entries in the map
        return rank;
    }

    CCL_THROW_IF_NOT(m_local2global_map.size() > rank,
                     "no rank ", rank, " was found in comm ", this, ", id ", m_id.value());
    size_t global_rank = m_local2global_map[rank];
    LOG_DEBUG("comm , ", this, " id ", m_id.value(), ", map rank ", rank, " to global ", global_rank);
    return global_rank;
}
