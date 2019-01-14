#include "common/comm/comm.hpp"
#include "common/log/log.hpp"
#include "mlsl_types.h"
#include "comm.hpp"

#include <vector>
#include <limits>
#include <iostream>
#include <mutex>

constexpr const size_t MLSL_COMM_ID_MAX = std::numeric_limits<mlsl_comm_id_t>::max();
constexpr const size_t MLSL_SCHED_ID_MAX = std::numeric_limits<mlsl_sched_id_t>::max();

/********************************************************************************
 *  atl tag                                                                     *
 * ******************************************************************************
 * 01234567 01234567 | 01234567 01234567  | 01234567 01234567 01234567 01234567 |
 *                                        |                                     |
 *      comm_id      | sched_id(per comm) |            global rank              |
 ********************************************************************************/
//number of bits used to shift global rank
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_SHIFT = 0;
//number of bits used to shift schedule id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_SHIFT = 32;
//number of bits used to shift comm id
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_SHIFT = 48;
//mask to filter global rank after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_RANK_MASK     = 0x00000000FFFFFFFF;
//mask to filter sched id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_SCHED_ID_MASK = 0x0000FFFF00000000;
//mask to filter comm id after shifting
constexpr const mlsl_atl_comm_tag_t MLSL_ATL_TAG_COMM_ID_MASK  = 0xFFFF000000000000;

class comm_id_storage
{
public:
    comm_id_storage()
    {
        free_ids.resize(MLSL_COMM_ID_MAX, true);
    }

    mlsl_status_t acquire_id(mlsl_comm_id_t& result)
    {
        std::lock_guard<std::mutex> lock (sync_guard);
        //search from the current position till the end
        for (size_t i = last_used; i < MLSL_COMM_ID_MAX; ++i)
        {
            if (free_ids[i])
            {
                free_ids[i] = false;
                last_used = i;
                result = i;
                MLSL_LOG(DEBUG, "Found free comm id %hu", result);
                return mlsl_status_success;
            }
        }

        //if we didn't exit from the method than there are no free ids in range [last_used:MLSL_COMM_ID_MAX)
        //need to repeat from the beginning of the ids space [0:last_used)
        for (size_t i = 0; i < last_used; ++i)
        {
            if (free_ids[i])
            {
                free_ids[i] = false;
                last_used = i;
                result = i;
                MLSL_LOG(DEBUG, "Found free comm id %hu", result);
                return mlsl_status_success;
            }
        }

        //nothing was found
        MLSL_LOG(INFO, "No free comm ids were found");
        return mlsl_status_out_of_resource;
    }

    mlsl_status_t release_id(mlsl_comm_id_t id)
    {
        std::lock_guard<std::mutex> lock (sync_guard);
        if (free_ids[id])
        {
            MLSL_LOG(ERROR, "Attempt to release not acquired id %hu", id);
            return mlsl_status_invalid_arguments;
        }
        free_ids[id] = true;
        last_used = id;

        return mlsl_status_success;
    }

private:
    size_t last_used {};
    std::vector<bool> free_ids;
    std::mutex sync_guard;
};

static comm_id_storage id_storage;

mlsl_status_t mlsl_comm_id_acquire(mlsl_comm_id_t* out_id)
{
    MLSL_ASSERT(out_id);
    return id_storage.acquire_id(*out_id);
}

mlsl_status_t mlsl_comm_id_release(mlsl_comm_id_t id)
{
    return id_storage.release_id(id);
}

mlsl_sched_id_t mlsl_comm_get_sched_id(mlsl_comm *comm)
{
    MLSL_ASSERT(comm);

    mlsl_sched_id_t id = comm->next_sched_id;

    ++comm->next_sched_id;

    if (comm->next_sched_id == MLSL_SCHED_ID_MAX)
    {
        /* wrap the sched numbers around to the start */
        comm->next_sched_id = 0;
    }

    MLSL_LOG(DEBUG, "sched_id %u, comm_id %hu, next sched_id %hu", id, comm->comm_id,
             comm->next_sched_id);

    return id;
}

mlsl_atl_comm_tag_t mlsl_create_atl_tag(mlsl_comm_id_t comm_id, mlsl_sched_id_t sched_id, size_t rank)
{
    mlsl_atl_comm_tag_t tag = 0;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(comm_id)  << MLSL_ATL_TAG_COMM_ID_SHIFT)  & MLSL_ATL_TAG_COMM_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(sched_id) << MLSL_ATL_TAG_SCHED_ID_SHIFT) & MLSL_ATL_TAG_SCHED_ID_MASK;
    tag |= (static_cast<mlsl_atl_comm_tag_t>(rank)     << MLSL_ATL_TAG_RANK_SHIFT)     & MLSL_ATL_TAG_RANK_MASK;

    MLSL_LOG(DEBUG, "comm %u, sched_id %u, rank %zu, tag %lu", comm_id, sched_id, rank, tag);
    return tag;
}
