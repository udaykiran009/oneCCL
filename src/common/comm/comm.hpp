#pragma once

#include "mlsl.hpp"
#include "common/comm/comm_id.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/log/log.hpp"

#include <unordered_map>
#include <atomic>

//key = rank, value = global rank
using rank_to_global_rank_map = std::unordered_map<size_t, size_t>;

class mlsl_comm
{
public:
    mlsl_comm() = delete;
    mlsl_comm(const mlsl_comm& other) = delete;
    mlsl_comm& operator=(const mlsl_comm& other) = delete;

    mlsl_comm(size_t rank, size_t size, std::unique_ptr<comm_id> id);
    mlsl_comm(size_t rank, size_t size, std::unique_ptr<comm_id> id, rank_to_global_rank_map&& ranks);

    ~mlsl_comm();

    static mlsl_comm* create_with_color(int color, comm_id_storage* comm_ids, const mlsl_comm* global_comm);

    std::shared_ptr<mlsl_comm> clone_with_new_id(std::unique_ptr<comm_id> id);

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

    mlsl_comm_id_t id() const
    {
        return m_id->value();
    }

    mlsl_comm_id_t get_sched_id()
    {
        mlsl_sched_id_t id = m_next_sched_id;

        ++m_next_sched_id;

        if (m_next_sched_id == mlsl_comm::max_sched_count)
        {
            /* wrap the sched numbers around to the start */
            m_next_sched_id = 0;
        }

        MLSL_LOG(DEBUG, "sched_id %u, comm_id %hu, next sched_id %hu", id, m_id->value(), m_next_sched_id);

        return id;
    }

    /**
     * Returns the number of @c rank in the global communicator
     * @param rank a rank which is part of the current communicator
     * @return number of @c rank in the global communicator
     */
    size_t get_global_rank(size_t rank) const;

    /**
     * Total number of active communicators
     */
    static std::atomic_size_t comm_count;
    /**
     * Maximum available number of active communicators
     */
    static constexpr const size_t max_comm_count = std::numeric_limits<mlsl_comm_id_t>::max();
    /**
     * Maximum value of schedule id in scope of the current communicator
     */
    static constexpr const size_t max_sched_count = std::numeric_limits<mlsl_sched_id_t>::max();

private:
    size_t m_rank;
    size_t m_size;
    size_t m_pof2;
    std::unique_ptr<comm_id> m_id;
    mlsl_comm_id_t m_next_sched_id = 0;
    rank_to_global_rank_map m_ranks_map{};
};
