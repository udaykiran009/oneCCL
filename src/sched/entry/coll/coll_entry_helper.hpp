#pragma once

#include "coll/selection/selection.hpp"
#include "sched/entry/coll/coll_entry_param.hpp"
#include "sched/entry/factory/entry_factory.hpp"

class coll_entry;

class coll_entry_helper
{
public:

    template<ccl_coll_type coll_id>
    static coll_entry* add_coll_entry(ccl_sched* sched,
                                      const ccl_coll_entry_param& param)
    {
        CCL_THROW_IF_NOT(coll_id == param.ctype);
        if (env_data.atl_transport == ccl_atl_mpi)
        {
            ccl_coll_param coll_param = param.to_coll_param();
            bool is_direct_algo = global_data.algorithm_selector->is_direct<coll_id>(coll_param);

            if (is_direct_algo)
            {
                if (sched->coll_attr.prologue_fn)
                {
                    /*
                        for direct MPI algo with prologue will use regular coll_entry
                        to simplify work with postponed fields
                    */
                    sched->strict_start_order = true;
                }
                else
                {
                    /* otherwise will place entry directly into schedule due to performance reasons */
                    auto res = coll_entry_helper::build_schedule(sched, sched, param);
                    CCL_ASSERT(res == ccl_status_success, "error during build_schedule, res ", res);
                    return nullptr; /* coll_entry ptr is required for prologue case only */
                }
            }
        }

        /* for remaining cases use regular coll_entry to get schedule filling offload */
        return entry_factory::make_entry<coll_entry>(sched, param);
    }

    static ccl_status_t build_schedule(ccl_sched* sched,
                                       const ccl_sched* parent_sched,
                                       const ccl_coll_entry_param& param);
};
