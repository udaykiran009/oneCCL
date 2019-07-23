#pragma once

#include "ccl_types.h"
#include "common/log/log.hpp"
#include "common/utils/spinlock.hpp"

#include <vector>
#include <limits>
#include <iostream>
#include <mutex>

using ccl_comm_id_t = uint16_t;

class ccl_comm_id_storage
{
public:
    explicit ccl_comm_id_storage(ccl_comm_id_t max_comm_count) : max_comm(max_comm_count),
                                                                   external_ids_range_start(max_comm >> 1),
                                                                   last_used_id_internal(),
                                                                   last_used_id_external(external_ids_range_start),
                                                                   free_ids(max_comm, true)
    {}

    ccl_comm_id_t acquire_id(bool internal = false)
    {
        std::lock_guard<ccl_spinlock> lock(sync_guard);
        ccl_comm_id_t& last_used_ref = internal ? last_used_id_internal : last_used_id_external;
        ccl_comm_id_t lower_bound = internal ? static_cast<ccl_comm_id_t>(0) : external_ids_range_start;
        ccl_comm_id_t upper_bound = internal ? external_ids_range_start : max_comm;

        LOG_DEBUG("looking for free ", internal ? "internal" : "external", " comm id");
        //overwrite last_used with new value
        last_used_ref = acquire_id_impl(last_used_ref, lower_bound, upper_bound);
        return last_used_ref;
    }

    /**
     * Forced way to obtain a specific comm id. Must be used with care
     */
    void pull_id(ccl_comm_id_t id)
    {
        CCL_ASSERT(id > 0 && id < max_comm, "id ", id, " is out of bounds");
        std::lock_guard<ccl_spinlock> lock(sync_guard);
        if (!free_ids[id])
        {
            CCL_THROW("comm id ", id, " is already used");
        }
        free_ids[id] = false;
    }

    void release_id(ccl_comm_id_t id)
    {
        std::lock_guard<ccl_spinlock> lock(sync_guard);
        if (free_ids[id])
        {
            LOG_ERROR("attempt to release not acquired id ", id);
            return;
        }
        LOG_DEBUG("free comm id ", id);
        free_ids[id] = true;
        last_used_id_internal = id;
    }

private:
    ccl_comm_id_t acquire_id_impl(ccl_comm_id_t last_used,
                                  ccl_comm_id_t lower_bound,
                                  ccl_comm_id_t upper_bound)
    {
        //search from the current position till the end
        LOG_DEBUG("last ", last_used, ", low ", lower_bound, " up ", upper_bound);
        for (ccl_comm_id_t id = last_used; id < upper_bound; ++id)
        {
            if (free_ids[id])
            {
                free_ids[id] = false;
                LOG_DEBUG("Found free comm id ", id);
                return id;
            }
        }

        //if we didn't exit from the method than there are no free ids in range [last_used:upper_bound)
        //need to repeat from the beginning of the ids space [lower_bound:last_used)
        for (ccl_comm_id_t id = lower_bound; id < last_used; ++id)
        {
            if (free_ids[id])
            {
                free_ids[id] = false;
                LOG_DEBUG("Found free comm id ", id);
                return id;
            }
        }

        throw ccl::ccl_error("no free comm id was found");
    }

    //max_comm space is split into 2 parts - internal and external
    const ccl_comm_id_t max_comm;
    //internal ids range [0, external_ids_range_start)
    //external ids range [external_ids_range_start, max_comm)
    const ccl_comm_id_t external_ids_range_start;
    ccl_comm_id_t last_used_id_internal{};
    ccl_comm_id_t last_used_id_external{};
    std::vector<bool> free_ids;
    ccl_spinlock sync_guard;
};
