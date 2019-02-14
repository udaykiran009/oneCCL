#pragma once

#include "common/log/log.hpp"
#include "mlsl_types.h"

#include <vector>
#include <limits>
#include <iostream>
#include <mutex>

using mlsl_comm_id_t = uint16_t;

class comm_id_storage
{
public:
    explicit comm_id_storage(size_t max_comm_count) : max_comm(max_comm_count), last_used(), free_ids(max_comm, true)
    {}

    mlsl_comm_id_t acquire_id()
    {
        std::lock_guard<std::mutex> lock (sync_guard);
        //search from the current position till the end
        for (mlsl_comm_id_t id = last_used; id < max_comm; ++id)
        {
            if (free_ids[id])
            {
                free_ids[id] = false;
                last_used = id;
                MLSL_LOG(DEBUG, "Found free comm id %hu", id);
                return id;
            }
        }

        //if we didn't exit from the method than there are no free ids in range [last_used:max_comm)
        //need to repeat from the beginning of the ids space [0:last_used)
        for (mlsl_comm_id_t id = 0; id < last_used; ++id)
        {
            if (free_ids[id])
            {
                free_ids[id] = false;
                last_used = id;
                MLSL_LOG(DEBUG, "Found free comm id %hu", id);
                return id;
            }
        }

        throw mlsl::mlsl_error("no free comm id was found");
    }

    void release_id(mlsl_comm_id_t id)
    {
        std::lock_guard<std::mutex> lock (sync_guard);
        if (free_ids[id])
        {
            MLSL_LOG(ERROR, "Attempt to release not acquired id %hu", id);
            return;
        }
        MLSL_LOG(DEBUG, "free comm id %hu", id);
        free_ids[id] = true;
        last_used = id;
    }

private:
    const size_t max_comm;
    mlsl_comm_id_t last_used {};
    std::vector<bool> free_ids;
    std::mutex sync_guard;
};
