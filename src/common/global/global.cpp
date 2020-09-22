#include "coll/algorithms/allreduce/allreduce_2d.hpp"
#include "coll/selection/selection.hpp"
#include "common/comm/atl_tag.hpp"
#include "common/comm/comm_id_storage.hpp"
#include "common/datatype/datatype.hpp"
#include "common/global/global.hpp"
#include "common/stream/stream.hpp"
#include "common/utils/tree.hpp"
#include "exec/exec.hpp"
#include "fusion/fusion.hpp"
#include "parallelizer/parallelizer.hpp"
#include "sched/cache/cache.hpp"
#include "unordered_coll/unordered_coll.hpp"

namespace ccl {

thread_local bool global_data::is_worker_thread = false;

global_data::global_data() {
    /* create ccl_logger before ccl::global_data
       to ensure static objects construction/destruction rule */
    LOG_INFO("create global_data object");

    //TODO new_api configure thread wait timeout
    extern size_t thread_barrier_wait_timeout_sec = 5;
}

global_data::~global_data() {
    reset();
}

global_data& global_data::get() {
    static global_data data;
    return data;
}

env_data& global_data::env() {
    return get().env_object;
}

ccl_status_t global_data::reset() {
    /*
        executor is resize_dependent object but out of regular reset procedure
        executor is responsible for resize logic and has own multi-step reset
     */
    executor.reset();
    global_ctx.reset();
    reset_resize_dependent_objects();
    reset_resize_independent_objects();

    return ccl_status_success;
}

ccl_status_t global_data::init() {
    env_object.parse();

    init_resize_dependent_objects();
    init_resize_independent_objects();

    if (executor->get_global_proc_idx() == 0)
        env_object.print();

    return ccl_status_success;
}

void global_data::init_resize_dependent_objects() {
    dtypes = std::unique_ptr<ccl_datatype_storage>(new ccl_datatype_storage());

    sched_cache = std::unique_ptr<ccl_sched_cache>(new ccl_sched_cache());

    if (env_object.enable_fusion) {
        /* create fusion_manager before executor because service_worker uses fusion_manager */
        fusion_manager = std::unique_ptr<ccl_fusion_manager>(new ccl_fusion_manager());
    }

    executor = std::unique_ptr<ccl_executor>(new ccl_executor());

    comm_ids =
        std::unique_ptr<ccl_comm_id_storage>(new ccl_comm_id_storage(ccl_comm::max_comm_count));

    comm = std::make_shared<ccl_comm>(executor->get_global_proc_idx(),
                                      executor->get_global_proc_count(),
                                      comm_ids->acquire(true));

    if (env_object.enable_unordered_coll) {
        unordered_coll_manager =
            std::unique_ptr<ccl_unordered_coll_manager>(new ccl_unordered_coll_manager());
    }

    allreduce_2d_builder = std::unique_ptr<ccl_allreduce_2d_builder>(new ccl_allreduce_2d_builder(
        (env_object.allreduce_2d_base_size != CCL_ENV_SIZET_NOT_SPECIFIED)
            ? env_object.allreduce_2d_base_size
            : executor->get_local_proc_count(),
        env_object.allreduce_2d_switch_dims));

    atl_tag = std::unique_ptr<ccl_atl_tag>(
        new ccl_atl_tag(executor->get_atl_attr().tag_bits, executor->get_atl_attr().max_tag));
    if (env_object.default_resizable)
        ccl_set_resize_fn(nullptr);

    if (executor->get_global_proc_idx() == 0) {
        atl_tag->print();
    }

    global_ctx = std::unique_ptr<ccl_group_context>(new ccl_group_context());
}

void global_data::init_resize_independent_objects() {
    parallelizer = std::unique_ptr<ccl_parallelizer>(new ccl_parallelizer(env_object.worker_count));

    algorithm_selector = std::unique_ptr<ccl_algorithm_selector_wrapper<CCL_COLL_LIST>>(
        new ccl_algorithm_selector_wrapper<CCL_COLL_LIST>());

    algorithm_selector->init();

    default_coll_attr.reset(new ccl_coll_attr_t{});
    memset(default_coll_attr.get(), 0, sizeof(ccl_coll_attr_t));

    bfp16_impl_type = ccl_bfp16_get_impl_type();

    if (executor->get_global_proc_idx() == 0) {
        algorithm_selector->print();

        if (bfp16_impl_type != ccl_bfp16_none) {
            LOG_INFO("\n\nBFP16 is enabled through ",
                     (bfp16_impl_type == ccl_bfp16_avx512bf) ? "AVX512-BF" : "AVX512-F",
                     "\n");
        }
        else {
#ifdef CCL_BFP16_COMPILER
            LOG_INFO("\n\nBFP16 is disabled on HW level\n");
#else
            LOG_INFO("\n\nBFP16 is disabled on compiler level\n");
#endif
        }
    }
}

void global_data::reset_resize_dependent_objects() {
    atl_tag.reset();
    allreduce_2d_builder.reset();
    unordered_coll_manager.reset();
    comm.reset();
    comm_ids.reset();
    fusion_manager.reset();
    sched_cache.reset();
    dtypes.reset();
}

void global_data::reset_resize_independent_objects() {
    parallelizer.reset();
    algorithm_selector.reset();
    default_coll_attr.reset();
}

} /* namespace ccl */
