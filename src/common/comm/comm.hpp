#pragma once

#include "iccl.hpp"
#include "common/comm/comm_id.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/log/log.hpp"
#include "common/utils/utils.hpp"
#include "common/utils/tree.hpp"

#include <unordered_map>
#include <atomic>

//key = rank, value = global rank
using rank_to_global_rank_map = std::unordered_map<size_t, size_t>;

class alignas(CACHELINE_SIZE) iccl_comm
{
public:
    iccl_comm() = delete;
    iccl_comm(const iccl_comm& other) = delete;
    iccl_comm& operator=(const iccl_comm& other) = delete;

    iccl_comm(size_t rank, size_t size, std::unique_ptr<comm_id> id);
    iccl_comm(size_t rank, size_t size, std::unique_ptr<comm_id> id, rank_to_global_rank_map&& ranks);

    ~iccl_comm() = default;

    static iccl_comm* create_with_color(int color, iccl_comm_id_storage* comm_ids, const iccl_comm* global_comm);

    std::shared_ptr<iccl_comm> clone_with_new_id(std::unique_ptr<comm_id> id);

    size_t rank() const
    {
        return m_rank;
    }

    size_t size() const
    {
        return m_size;
    }

    size_t pof2() const
    {
        return m_pof2;
    }

    iccl_comm_id_t id() const
    {
        return m_id->value();
    }

    iccl_sched_id_t get_sched_id(bool use_internal_space)
    {
        iccl_sched_id_t& next_sched_id = (use_internal_space) ? m_next_sched_id_internal : m_next_sched_id_external;
        iccl_sched_id_t first_sched_id = (use_internal_space) ? static_cast<iccl_sched_id_t>(0) : iccl_comm::max_sched_count / 2;
        iccl_sched_id_t max_sched_id = (use_internal_space) ? iccl_comm::max_sched_count / 2 : iccl_comm::max_sched_count ;

        iccl_sched_id_t id = next_sched_id;

        ++next_sched_id;

        if (next_sched_id == max_sched_id)
        {
            /* wrap the sched numbers around to the start */
            next_sched_id = first_sched_id;
        }

        LOG_DEBUG("sched_id ", id, ", comm_id ", m_id->value(), ", next sched_id ", next_sched_id);

        return id;
    }

    /**
     * Returns the number of @c rank in the global communicator
     * @param rank a rank which is part of the current communicator
     * @return number of @c rank in the global communicator
     */
    size_t get_global_rank(size_t rank) const;

    const double_tree& dtree() const
    {
        return m_dtree;
    }

    /**
     * Total number of active communicators
     */
    static std::atomic_size_t comm_count;
    /**
     * Maximum available number of active communicators
     */
    static constexpr iccl_sched_id_t max_comm_count = std::numeric_limits<iccl_comm_id_t>::max();
    /**
     * Maximum value of schedule id in scope of the current communicator
     */
    static constexpr iccl_sched_id_t max_sched_count = std::numeric_limits<iccl_sched_id_t>::max();

private:
    size_t m_rank;
    size_t m_size;
    size_t m_pof2;
    std::unique_ptr<comm_id> m_id;
    iccl_sched_id_t m_next_sched_id_internal = iccl_comm::max_sched_count / 2;
    iccl_sched_id_t m_next_sched_id_external = 0;
    rank_to_global_rank_map m_ranks_map{};
    double_tree m_dtree;
};
