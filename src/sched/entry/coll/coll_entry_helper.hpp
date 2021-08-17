#pragma once

#include "coll/selection/selection.hpp"
#include "sched/entry/coll/coll_entry_param.hpp"
#include "sched/entry/factory/entry_factory.hpp"

class coll_entry;

class coll_entry_helper {
public:
    template <ccl_coll_type coll_id>
    static coll_entry* add_coll_entry(ccl_sched* sched, const ccl_coll_entry_param& param) {
        CCL_THROW_IF_NOT(coll_id == param.ctype);

        ccl_selector_param selector_param;
        selector_param.ctype = param.ctype;
        selector_param.count = param.count;
        if (param.ctype == ccl_coll_allgatherv) {
            selector_param.count = param.send_count;
        }
        selector_param.recv_counts = param.recv_counts;
        selector_param.dtype = param.dtype;
        selector_param.comm = param.comm;
        selector_param.stream = param.stream;
        selector_param.is_vector_buf = sched->coll_attr.is_vector_buf;
#ifdef CCL_ENABLE_SYCL
        selector_param.is_sycl_buf = sched->coll_attr.is_sycl_buf;
#endif // CCL_ENABLE_SYCL
        selector_param.hint_algo = param.hint_algo;

        if (ccl_is_topo_ring_algo(selector_param)) {
            sched->strict_order = true;
        }

        if ((ccl::global_data::env().atl_transport == ccl_atl_mpi) &&
            ccl_is_direct_algo(selector_param)) {
            if (sched->coll_attr.prologue_fn) {
                /*
                    for direct MPI algo with prologue will use regular coll_entry
                    to simplify work with postponed fields
                */
                sched->strict_order = true;
            }
            else {
                /* otherwise will place entry directly into schedule due to performance reasons */
                auto res = coll_entry_helper::build_schedule(sched, sched, param);
                CCL_ASSERT(res == ccl::status::success, "error during build_schedule, res ", res);
                return nullptr; /* coll_entry ptr is required for prologue case only */
            }
        }

        /* for remaining cases use regular coll_entry to get schedule filling offload */
        return entry_factory::make_entry<coll_entry>(sched, param);
    }

    static ccl::status build_schedule(ccl_sched* sched,
                                      const ccl_sched* parent_sched,
                                      const ccl_coll_entry_param& param);
};
