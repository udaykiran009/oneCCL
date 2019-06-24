#include "common/comm/comm.hpp"
#include "sched/sched.hpp"
#include "coll/coll_algorithms.hpp"

std::atomic_size_t mlsl_comm::comm_count{};

mlsl_comm::mlsl_comm(size_t rank,
                     size_t size,
                     std::unique_ptr<comm_id> id) : mlsl_comm(rank, size, std::move(id), rank_to_global_rank_map{})
{}

mlsl_comm::mlsl_comm(size_t rank,
                     size_t size,
                     std::unique_ptr<comm_id> id,
                     rank_to_global_rank_map&& ranks) :
    m_rank(rank),
    m_size(size),
    m_pof2(mlsl_pof2(m_size)),
    m_id(std::move(id)),
    m_ranks_map(std::move(ranks)),
    m_dtree(size, rank)
{
}

static mlsl_status_t mlsl_comm_exchange_colors(std::vector<int>& colors)
{
    const size_t exchange_count = 1;
    std::vector<size_t> recv_counts(colors.size(), exchange_count);
    mlsl_coll_attr_t coll_attr{};
    coll_attr.to_cache = false;
    mlsl_request_t request;

    mlsl_status_t status;
    MLSL_CALL(mlsl_allgatherv(colors.data(), exchange_count,
                              colors.data(), recv_counts.data(),
                              mlsl_dtype_int, &coll_attr,
                              nullptr, &request));

    //wait for completion
    MLSL_CALL(mlsl_wait(request));

    return status;
}

mlsl_comm* mlsl_comm::create_with_color(int color,
                                        mlsl_comm_id_storage* comm_ids,
                                        const mlsl_comm* global_comm)
{
    mlsl_status_t status = mlsl_status_success;
    std::vector<int> all_colors(global_comm->size());
    all_colors[global_comm->rank()] = color;

    status = mlsl_comm_exchange_colors(all_colors);
    if (status != mlsl_status_success)
    {
        throw mlsl::mlsl_error("failed to exchange colors during comm creation");
    }

    size_t colors_match = 0;
    size_t new_rank = 0;

    rank_to_global_rank_map ranks_map;
    for (size_t i = 0; i < global_comm->size(); ++i)
    {
        if (all_colors[i] == color)
        {
            LOG_DEBUG("map local rank ", colors_match, " to global ", i);
            ranks_map[colors_match] = i;
            ++colors_match;
            if (i < global_comm->rank())
            {
                ++new_rank;
            }
        }
    }
    if (colors_match == 0)
    {
        throw mlsl::mlsl_error(
            std::string("no colors matched to ") + std::to_string(color) + " seems to be exchange issue");
    }

    if (colors_match == global_comm->size())
    {
        //Exact copy of the global communicator, use empty map
        ranks_map.clear();
    }

    mlsl_comm* comm = new mlsl_comm(new_rank, colors_match, std::unique_ptr<comm_id>(new comm_id(*comm_ids)),
                                    std::move(ranks_map));
    LOG_DEBUG("New comm: color ", color, ", rank ", comm->rank(), ", size ", comm->size(), ", comm_id ", comm->id());

    return comm;
}

std::shared_ptr<mlsl_comm> mlsl_comm::clone_with_new_id(std::unique_ptr<comm_id> id)
{
    rank_to_global_rank_map ranks_copy{m_ranks_map};
    return std::make_shared<mlsl_comm>(m_rank, m_size, std::move(id), std::move(ranks_copy));
}

size_t mlsl_comm::get_global_rank(size_t rank) const
{
    if (m_ranks_map.empty())
    {
        //global comm and its copies do not have entries in the map
        LOG_DEBUG("Direct mapping of rank ", rank);
        return rank;
    }

    auto result = m_ranks_map.find(rank);
    if (result == m_ranks_map.end())
    {
        MLSL_THROW("no rank ", rank, " was found in comm ", this, ", id ", m_id->value());
    }

    LOG_DEBUG("comm , ", this, " id ", m_id->value(), ", map rank ", rank, " to global ", result->second);
    return result->second;
}
