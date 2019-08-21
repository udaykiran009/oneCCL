#pragma once

#include "ccl.hpp"
#include "common/comm/comm_id_storage.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/log/log.hpp"
#include "common/utils/utils.hpp"
#include "common/utils/tree.hpp"

#include <unordered_map>
#include <atomic>

//key = rank, value = global rank
using rank_to_global_rank_map = std::unordered_map<size_t, size_t>;

class alignas(CACHELINE_SIZE) ccl_comm
{
public:
    ccl_comm() = delete;
    ccl_comm(const ccl_comm& other) = delete;
    ccl_comm& operator=(const ccl_comm& other) = delete;

    ccl_comm(size_t rank, size_t size, ccl_comm_id_storage::comm_id &&id);
    ccl_comm(size_t rank, size_t size, ccl_comm_id_storage::comm_id &&id, rank_to_global_rank_map&& ranks);

    ~ccl_comm() = default;

    static ccl_comm* create_with_color(int color, ccl_comm_id_storage* comm_ids, const ccl_comm* global_comm);

    std::shared_ptr<ccl_comm> clone_with_new_id(ccl_comm_id_storage::comm_id &&id);

    size_t rank() const noexcept
    {
        return m_rank;
    }

    size_t size() const noexcept
    {
        return m_size;
    }

    size_t pof2() const noexcept
    {
        return m_pof2;
    }

    ccl_comm_id_t id() const noexcept
    {
        return m_id.value();
    }

    ccl_sched_id_t get_sched_id(bool use_internal_space)
    {
        ccl_sched_id_t& next_sched_id = (use_internal_space) ? m_next_sched_id_internal : m_next_sched_id_external;
        ccl_sched_id_t first_sched_id = (use_internal_space) ? static_cast<ccl_sched_id_t>(0) : ccl_comm::max_sched_count / 2;
        ccl_sched_id_t max_sched_id = (use_internal_space) ? ccl_comm::max_sched_count / 2 : ccl_comm::max_sched_count ;

        ccl_sched_id_t id = next_sched_id;

        ++next_sched_id;

        if (next_sched_id == max_sched_id)
        {
            /* wrap the sched numbers around to the start */
            next_sched_id = first_sched_id;
        }

        LOG_DEBUG("sched_id ", id, ", comm_id ", m_id.value(), ", next sched_id ", next_sched_id);

        return id;
    }

    /**
     * Returns the number of @c rank in the global communicator
     * @param rank a rank which is part of the current communicator
     * @return number of @c rank in the global communicator
     */
    size_t get_global_rank(size_t rank) const;

    const ccl_double_tree& dtree() const
    {
        /* TODO: why we need double tree in communicator class? */
        return m_dtree;
    }

    /**
     * Total number of active communicators
     */
    static std::atomic_size_t comm_count;
    /**
     * Maximum available number of active communicators
     */
    static constexpr ccl_sched_id_t max_comm_count = std::numeric_limits<ccl_comm_id_t>::max();
    /**
     * Maximum value of schedule id in scope of the current communicator
     */
    static constexpr ccl_sched_id_t max_sched_count = std::numeric_limits<ccl_sched_id_t>::max();

private:
    size_t m_rank;
    size_t m_size;
    size_t m_pof2;

    ccl_comm_id_storage::comm_id m_id;
    ccl_sched_id_t m_next_sched_id_internal = ccl_comm::max_sched_count / 2;
    ccl_sched_id_t m_next_sched_id_external = 0;

    rank_to_global_rank_map m_ranks_map{};
    ccl_double_tree m_dtree;
};
