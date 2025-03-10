#include <climits>
#include <dlfcn.h>
#include <iterator>
#include <memory>
#include <sstream>
#include <unistd.h>

#include "coll/selection/selection.hpp"
#include "common/env/env.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "exec/exec.hpp"
#include "oneapi/ccl/environment.hpp"
#include "common/utils/version.hpp"

namespace ccl {

std::map<ccl_priority_mode, std::string> env_data::priority_mode_names = {
    std::make_pair(ccl_priority_none, "none"),
    std::make_pair(ccl_priority_direct, "direct"),
    std::make_pair(ccl_priority_lifo, "lifo")
};

std::map<ccl_atl_transport, std::string> env_data::atl_transport_names = {
    std::make_pair(ccl_atl_ofi, "ofi")
#ifdef CCL_ENABLE_MPI
        ,
    std::make_pair(ccl_atl_mpi, "mpi")
#endif // CCL_ENABLE_MPI
};

std::map<ccl_atl_send_proxy, std::string> env_data::atl_send_proxy_names = {
    std::make_pair(ccl_atl_send_proxy_none, "none"),
    std::make_pair(ccl_atl_send_proxy_regular, "regular"),
    std::make_pair(ccl_atl_send_proxy_usm, "usm")
};

std::map<ccl_staging_buffer, std::string> env_data::staging_buffer_names = {
    std::make_pair(ccl_staging_regular, "regular"),
    std::make_pair(ccl_staging_usm, "usm")
};

std::map<backend_mode, std::string> env_data::backend_names = {
    std::make_pair(backend_mode::native, "native"),
#ifdef CCL_ENABLE_STUB_BACKEND
    std::make_pair(backend_mode::stub, "stub")
#endif // CCL_ENABLE_STUB_BACKEND
};

std::map<process_launcher_mode, std::string> env_data::process_launcher_names = {
    std::make_pair(process_launcher_mode::hydra, "hydra"),
    std::make_pair(process_launcher_mode::torch, "torch"),
    std::make_pair(process_launcher_mode::none, "none")
};

env_data::env_data()
        : was_printed(false),

          log_level(ccl_log_level::warn),
          abort_on_throw(0),
          queue_dump(0),
          sched_dump(0),
          sched_profile(0),
          entry_max_update_time_sec(CCL_ENV_SIZET_NOT_SPECIFIED),

          fw_type(ccl_framework_none),

          worker_count(1),
          worker_offload(1),
          worker_wait(1),
#ifdef CCL_ENABLE_MPI
          atl_transport(ccl_atl_mpi),
#else // CCL_ENABLE_MPI
          atl_transport(ccl_atl_ofi),
#endif // CCL_ENABLE_MPI
          enable_shm(0),
          enable_rma(0),
          enable_hmem(0),
          atl_send_proxy(ccl_atl_send_proxy_none),
          enable_atl_cache(1),
          enable_sync_coll(0),
          enable_extra_ep(0),

          mnic_type(ATL_MNIC_NONE),
          mnic_count(CCL_ENV_SIZET_NOT_SPECIFIED),
          mnic_offset(ATL_MNIC_OFFSET_NONE),

          enable_algo_fallback(1),
          enable_unordered_coll(0),

          enable_fusion(0),
          fusion_bytes_threshold(16384),
          fusion_count_threshold(256),
          fusion_check_urgent(1),
          fusion_cycle_ms(0.2),

          priority_mode(ccl_priority_none),
          spin_count(100),
          yield_type(ccl_yield_pause),
          max_short_size(0),
          bcast_part_count(CCL_ENV_SIZET_NOT_SPECIFIED),
          cache_key_type(ccl_cache_key_match_id),
#ifdef CCL_ENABLE_SYCL
          enable_cache_flush(1),
#else // CCL_ENABLE_SYCL
          enable_cache_flush(0),
#endif // CCL_ENABLE_SYCL
          enable_buffer_cache(1),
          enable_strict_order(0),
          staging_buffer(ccl_staging_regular),
          enable_op_sync(0),
          enable_external_queue(0),

          chunk_count(1),
          min_chunk_size(65536),
          rs_chunk_count(1),
          rs_min_chunk_size(65536),

#ifdef CCL_ENABLE_SYCL
          allgatherv_topo_large_scale(0),
#endif // CCL_ENABLE_SYCL

          allreduce_nreduce_buffering(0),
          allreduce_nreduce_segment_size(CCL_ENV_SIZET_NOT_SPECIFIED),

          allreduce_2d_chunk_count(1),
          allreduce_2d_min_chunk_size(65536),
          allreduce_2d_switch_dims(0),

          alltoall_scatter_max_ops(CCL_ENV_SIZET_NOT_SPECIFIED),

          backend(backend_mode::native),

          local_rank(CCL_ENV_INT_NOT_SPECIFIED),
          local_size(CCL_ENV_INT_NOT_SPECIFIED),

          process_launcher(process_launcher_mode::hydra),

          enable_topo_algo(1),
#ifdef CCL_ENABLE_SYCL
          topo_color(topo_color_mode::ze),
#else // CCL_ENABLE_SYCL
          topo_color(topo_color_mode::fixed),
#endif // CCL_ENABLE_SYCL
          enable_p2p_access(CCL_ENV_INT_NOT_SPECIFIED),

#ifdef CCL_ENABLE_MPI
          mpi_lib_path(),
#endif // CCL_ENABLE_MPI
          ofi_lib_path(),

#ifdef CCL_ENABLE_SYCL
          kernel_path(),
          kernel_debug(0),
          kernel_group_size(32),
          kernel_group_count(CCL_ENV_SIZET_NOT_SPECIFIED),
          enable_kernel_sync(1),
          kernel_1s_lead(0),
          enable_kernel_1s_copy_ops(0),
          enable_kernel_1s_ipc_wa(0),
          enable_close_fd_wa(1),

          enable_sycl_output_event(0),
          use_hmem(1),

          enable_ze_barrier(0),
          enable_ze_bidir_algo(0),
          enable_ze_cache(1),
          enable_ze_cache_ipc_handles(1),
          ze_cache_ipc_handles_threshold(100),
          enable_ze_single_list(1),
          disable_ze_family_check(0),
          disable_ze_port_check(0),
          ze_serialize_mode(0),
          ze_copy_engine(ccl::ze::copy_engine_mode::link),
          ze_max_compute_queues(1),
          ze_max_copy_queues(CCL_ENV_SIZET_NOT_SPECIFIED),
          ze_enable_ccs_fallback_for_copy(1),
          enable_ze_list_dump(0),
          ze_queue_index_offset(1),
          ze_close_ipc_wa(0),
          ze_lib_path(),
          ze_enable(1),
          ze_fini_wa(0),
          ze_multi_workers(0),
#ifdef CCL_ENABLE_DRM
          ze_ipc_exchange(ccl::ze::ipc_exchange_mode::drmfd),
#else // CCL_ENABLE_DRM
          ze_ipc_exchange(ccl::ze::ipc_exchange_mode::sockets),
#endif // CCL_ENABLE_DRM
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_ITT
          itt_level(0),
#endif // CCL_ENABLE_ITT

          bf16_impl_type(ccl_bf16_scalar),
          fp16_impl_type(ccl_fp16_no_compiler_support) {
}

void env_data::parse() {
    env_2_enum(CCL_LOG_LEVEL, ccl_logger::level_names, log_level);
    env_2_type(CCL_ABORT_ON_THROW, abort_on_throw);
    ccl_logger::set_log_level(log_level);
    ccl_logger::set_abort_on_throw(abort_on_throw);
    env_2_type(CCL_QUEUE_DUMP, queue_dump);
    env_2_type(CCL_SCHED_DUMP, sched_dump);
    env_2_type(CCL_SCHED_PROFILE, sched_profile);
    env_2_type(CCL_ENTRY_MAX_UPDATE_TIME_SEC, entry_max_update_time_sec);
    CCL_THROW_IF_NOT(
        entry_max_update_time_sec == CCL_ENV_SIZET_NOT_SPECIFIED || entry_max_update_time_sec > 0,
        "incorrect ",
        CCL_ENTRY_MAX_UPDATE_TIME_SEC,
        " ",
        entry_max_update_time_sec);

    if (fw_type == ccl_framework_none) {
        /* try to automatically detect framework */
        void* handle = dlopen(NULL, RTLD_GLOBAL | RTLD_NOW);
        if (handle) {
            horovod_init_function =
                (ccl_horovod_init_function)dlsym(handle, horovod_init_function_name);
            dlclose(handle);
        }

        if (horovod_init_function) {
            LOG_INFO("found horovod_init function");
            fw_type = ccl_framework_horovod;
        }
    }
    env_2_enum(CCL_FRAMEWORK, ccl_framework_type_names, fw_type);

    if (fw_type == ccl_framework_horovod) {
        // enable_sync_coll = 1;
        // enable_extra_ep = 1;
        worker_wait = 1;
        yield_type = ccl_yield_sched_yield;
    }

    env_2_type(CCL_WORKER_COUNT, worker_count);
    CCL_THROW_IF_NOT(worker_count >= 1, "incorrect ", CCL_WORKER_COUNT, " ", worker_count);
    env_2_type(CCL_WORKER_OFFLOAD, worker_offload);
    env_2_type(CCL_WORKER_WAIT, worker_wait);

    env_2_atl_transport();
    env_2_type(CCL_ATL_SHM, enable_shm);
    env_2_type(CCL_ATL_RMA, enable_rma);
    env_2_type(CCL_ATL_HMEM, enable_hmem);
    if (atl_transport == ccl_atl_mpi && enable_hmem) {
        LOG_INFO("atl hmem requested, switch to single worker");
        worker_count = 1;
    }
    env_2_enum(CCL_ATL_SEND_PROXY, atl_send_proxy_names, atl_send_proxy);
    env_2_type(CCL_ATL_CACHE, enable_atl_cache);
    env_2_type(CCL_ATL_SYNC_COLL, enable_sync_coll);
    env_2_type(CCL_ATL_EXTRA_EP, enable_extra_ep);

    env_2_enum(CCL_MNIC, mnic_type_names, mnic_type);
    env_2_type(CCL_MNIC_NAME, mnic_name_raw);
    env_2_type(CCL_MNIC_COUNT, mnic_count);
    if (mnic_count == CCL_ENV_SIZET_NOT_SPECIFIED) {
        mnic_count = worker_count;
    }
    env_2_enum(CCL_MNIC_OFFSET, mnic_offset_names, mnic_offset);

    env_2_type(CCL_ALGO_FALLBACK, enable_algo_fallback);
    env_2_type(CCL_ALLGATHERV, allgatherv_algo_raw);
    env_2_type(CCL_ALLREDUCE, allreduce_algo_raw);
    env_2_type(CCL_ALLTOALL, alltoall_algo_raw);
    env_2_type(CCL_ALLTOALLV, alltoallv_algo_raw);
    env_2_type(CCL_BARRIER, barrier_algo_raw);
    env_2_type(CCL_BCAST, bcast_algo_raw);
    env_2_type(CCL_REDUCE, reduce_algo_raw);
    env_2_type(CCL_REDUCE_SCATTER, reduce_scatter_algo_raw);
    env_2_type(CCL_UNORDERED_COLL, enable_unordered_coll);
    if (enable_unordered_coll && atl_transport != ccl_atl_ofi) {
        CCL_THROW("unordered collectives are supported for OFI transport only");
    }

    env_2_type(CCL_FUSION, enable_fusion);
    env_2_type(CCL_FUSION_BYTES_THRESHOLD, fusion_bytes_threshold);
    env_2_type(CCL_FUSION_COUNT_THRESHOLD, fusion_count_threshold);
    env_2_type(CCL_FUSION_CHECK_URGENT, fusion_check_urgent);
    env_2_type(CCL_FUSION_CYCLE_MS, fusion_cycle_ms);
    if (enable_fusion) {
        CCL_THROW_IF_NOT(fusion_bytes_threshold >= 1,
                         "incorrect ",
                         CCL_FUSION_BYTES_THRESHOLD,
                         " ",
                         fusion_bytes_threshold);
        CCL_THROW_IF_NOT(fusion_count_threshold >= 1,
                         "incorrect ",
                         CCL_FUSION_COUNT_THRESHOLD,
                         " ",
                         fusion_count_threshold);
    }

    if (!worker_offload || enable_fusion)
        worker_wait = 0;

    if (worker_wait)
        spin_count = 1000;

    env_2_enum(CCL_PRIORITY, priority_mode_names, priority_mode);
    env_2_type(CCL_SPIN_COUNT, spin_count);
    env_2_enum(CCL_YIELD, ccl_yield_type_names, yield_type);
    env_2_type(CCL_MAX_SHORT_SIZE, max_short_size);
    env_2_type(CCL_BCAST_PART_COUNT, (size_t&)bcast_part_count);
    env_2_enum(CCL_CACHE_KEY, ccl_sched_key::key_type_names, cache_key_type);
    env_2_type(CCL_CACHE_FLUSH, enable_cache_flush);
    env_2_type(CCL_BUFFER_CACHE, enable_buffer_cache);
    env_2_type(CCL_STRICT_ORDER, enable_strict_order);
    if (enable_unordered_coll && enable_strict_order) {
        LOG_INFO("unordered collectives are requested, disable strict order");
        enable_strict_order = 0;
    }
    env_2_enum(CCL_STAGING_BUFFER, staging_buffer_names, staging_buffer);
    env_2_type(CCL_OP_SYNC, enable_op_sync);
    env_2_type(CCL_USE_EXTERNAL_QUEUE, enable_external_queue);

    env_2_type(CCL_CHUNK_COUNT, chunk_count);
    CCL_THROW_IF_NOT(chunk_count >= 1, "incorrect ", CCL_CHUNK_COUNT, " ", chunk_count);
    env_2_type(CCL_MIN_CHUNK_SIZE, min_chunk_size);
    CCL_THROW_IF_NOT(min_chunk_size >= 1, "incorrect ", CCL_MIN_CHUNK_SIZE, " ", min_chunk_size);

    env_2_type(CCL_RS_CHUNK_COUNT, rs_chunk_count);
    CCL_THROW_IF_NOT(rs_chunk_count >= 1, "incorrect ", CCL_RS_CHUNK_COUNT, " ", rs_chunk_count);
    env_2_type(CCL_RS_MIN_CHUNK_SIZE, rs_min_chunk_size);
    CCL_THROW_IF_NOT(
        rs_min_chunk_size >= 1, "incorrect ", CCL_RS_MIN_CHUNK_SIZE, " ", rs_min_chunk_size);

#ifdef CCL_ENABLE_SYCL
    env_2_type(CCL_ALLGATHERV_TOPO_LARGE_SCALE, allgatherv_topo_large_scale);
#endif // CCL_ENABLE_SYCL

    env_2_type(CCL_ALLREDUCE_NREDUCE_BUFFERING, allreduce_nreduce_buffering);
    env_2_type(CCL_ALLREDUCE_NREDUCE_SEGMENT_SIZE, (size_t&)allreduce_nreduce_segment_size);

    env_2_type(CCL_ALLREDUCE_2D_CHUNK_COUNT, allreduce_2d_chunk_count);
    CCL_THROW_IF_NOT(allreduce_2d_chunk_count >= 1,
                     "incorrect ",
                     CCL_ALLREDUCE_2D_CHUNK_COUNT,
                     " ",
                     allreduce_2d_chunk_count);
    env_2_type(CCL_ALLREDUCE_2D_MIN_CHUNK_SIZE, allreduce_2d_min_chunk_size);
    CCL_THROW_IF_NOT(allreduce_2d_min_chunk_size >= 1,
                     "incorrect ",
                     CCL_ALLREDUCE_2D_MIN_CHUNK_SIZE,
                     " ",
                     allreduce_2d_min_chunk_size);
    env_2_type(CCL_ALLREDUCE_2D_SWITCH_DIMS, allreduce_2d_switch_dims);

    env_2_type(CCL_ALLTOALL_SCATTER_MAX_OPS, (size_t&)alltoall_scatter_max_ops);

    env_2_enum(CCL_BACKEND, backend_names, backend);

    env_2_type(CCL_LOCAL_RANK, local_rank);
    env_2_type(CCL_LOCAL_SIZE, local_size);

    env_2_enum(CCL_PROCESS_LAUNCHER, process_launcher_names, process_launcher);

    env_2_type(CCL_TOPO_ALGO, enable_topo_algo);
    env_2_topo(CCL_TOPO_COLOR, topo_color_names, topo_color);
    env_2_type(CCL_TOPO_P2P_ACCESS, enable_p2p_access);

#ifdef CCL_ENABLE_MPI
    env_2_type(CCL_MPI_LIBRARY_PATH, mpi_lib_path);
#endif // CCL_ENABLE_MPI
    env_2_type(CCL_OFI_LIBRARY_PATH, ofi_lib_path);

#ifdef CCL_ENABLE_SYCL
    env_2_type(CCL_KERNEL_PATH, kernel_path);
    if (kernel_path.empty()) {
        std::string ccl_root;
        char* ccl_root_env_value = getenv("CCL_ROOT");
        if (ccl_root_env_value) {
            ccl_root = ccl_root_env_value;
        }
        CCL_THROW_IF_NOT(!ccl_root.empty(), "incorrect comm kernels path, CCL_ROOT not found!");
        kernel_path = ccl_root + "/lib/kernels/";
    }

    env_2_type(CCL_KERNEL_DEBUG, kernel_debug);
    env_2_type(CCL_KERNEL_GROUP_SIZE, kernel_group_size);
    env_2_type(CCL_KERNEL_GROUP_COUNT, kernel_group_count);
    env_2_type(CCL_KERNEL_SYNC, enable_kernel_sync);
    env_2_type(CCL_KERNEL_1S_LEAD, kernel_1s_lead);
    env_2_type(CCL_KERNEL_1S_USE_COPY_OPS, enable_kernel_1s_copy_ops);
    env_2_type(CCL_KERNEL_1S_IPC_WA, enable_kernel_1s_ipc_wa);
    env_2_type(CCL_KERNEL_CLOSE_FD_WA, enable_close_fd_wa);

    env_2_type(CCL_SYCL_OUTPUT_EVENT, enable_sycl_output_event);
    env_2_type(CCL_USE_HMEM, use_hmem);

    env_2_type(CCL_ZE_BARRIER, enable_ze_barrier);
    env_2_type(CCL_ZE_BIDIR_ALGO, enable_ze_bidir_algo);
    env_2_type(CCL_ZE_CACHE, enable_ze_cache);
    env_2_type(CCL_ZE_CACHE_IPC_HANDLES, enable_ze_cache_ipc_handles);
    env_2_type(CCL_ZE_CACHE_IPC_HANDLES_THRESHOLD, ze_cache_ipc_handles_threshold);
    if (enable_ze_cache == 0) {
        enable_ze_cache_ipc_handles = 0;
    }
    else if (enable_ze_cache && enable_ze_cache_ipc_handles) {
        CCL_THROW_IF_NOT(ze_cache_ipc_handles_threshold > 0,
                         "incorrect ",
                         CCL_ZE_CACHE_IPC_HANDLES_THRESHOLD,
                         " ",
                         ze_cache_ipc_handles_threshold);
    }
    env_2_type(CCL_ZE_SINGLE_LIST, enable_ze_single_list);
    env_2_type(CCL_ZE_DISABLE_FAMILY_CHECK, disable_ze_family_check);
    env_2_type(CCL_ZE_DISABLE_PORT_CHECK, disable_ze_port_check);
    env_2_type(CCL_ZE_SERIALIZE, ze_serialize_mode);
    env_2_enum(CCL_ZE_COPY_ENGINE, ccl::ze::copy_engine_names, ze_copy_engine);
    env_2_type(CCL_ZE_MAX_COMPUTE_QUEUES, ze_max_compute_queues);
    CCL_THROW_IF_NOT(
        ze_max_compute_queues == CCL_ENV_SIZET_NOT_SPECIFIED || ze_max_compute_queues > 0,
        "incorrect ",
        CCL_ZE_MAX_COMPUTE_QUEUES,
        " ",
        ze_max_compute_queues);
    env_2_type(CCL_ZE_MAX_COPY_QUEUES, ze_max_copy_queues);
    CCL_THROW_IF_NOT(ze_copy_engine == ccl::ze::copy_engine_mode::none ||
                         ze_max_copy_queues == CCL_ENV_SIZET_NOT_SPECIFIED ||
                         ze_max_copy_queues > 0,
                     "incorrect ",
                     CCL_ZE_MAX_COPY_QUEUES,
                     " ",
                     ze_max_copy_queues);
    env_2_type(CCL_ZE_ENABLE_CCS_FALLBACK_FOR_COPY, ze_enable_ccs_fallback_for_copy);
    env_2_type(CCL_ZE_LIST_DUMP, enable_ze_list_dump);
    env_2_type(CCL_ZE_QUEUE_INDEX_OFFSET, ze_queue_index_offset);
    CCL_THROW_IF_NOT(ze_queue_index_offset >= 0,
                     "incorrect ",
                     CCL_ZE_QUEUE_INDEX_OFFSET,
                     " ",
                     ze_queue_index_offset);
    env_2_type(CCL_ZE_CLOSE_IPC_WA, ze_close_ipc_wa);
    env_2_type(CCL_ZE_LIBRARY_PATH, ze_lib_path);
    env_2_type(CCL_ZE_ENABLE, ze_enable);
    env_2_type(CCL_ZE_FINI_WA, ze_fini_wa);
    env_2_type(CCL_ZE_MULTI_WORKERS, ze_multi_workers);
    env_2_enum(CCL_ZE_IPC_EXCHANGE, ze::ipc_exchange_names, ze_ipc_exchange);
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_ITT
    env_2_type(CCL_ITT_LEVEL, itt_level);
#endif // CCL_ENABLE_ITT

    auto bf16_impl_types = ccl_bf16_get_impl_types();
    if (!env_2_enum(CCL_BF16, bf16_impl_names, bf16_impl_type)) {
        bf16_impl_type = *bf16_impl_types.rbegin();
    }

    auto fp16_impl_types = ccl_fp16_get_impl_types();
    ccl_fp16_impl_type fp16_env_impl_type;
    if (env_2_enum(CCL_FP16, fp16_env_impl_names, fp16_env_impl_type)) {
        CCL_THROW_IF_NOT(fp16_impl_types.find(fp16_env_impl_type) != fp16_impl_types.end(),
                         "unsupported FP16 impl type: ",
                         fp16_env_impl_names[fp16_env_impl_type]);
        fp16_impl_type = fp16_env_impl_type;
    }
    else {
        fp16_impl_type = *fp16_impl_types.rbegin();
    }
}

void env_data::print(int rank) {
    std::lock_guard<ccl_spinlock> lock{ print_guard };

    if (was_printed)
        return;
    else
        was_printed = true;

    auto& global_data = ccl::global_data::get();

    if (rank == 0) {
        auto version = utils::get_library_version();
        LOG_INFO("library version: ", version.full);
        LOG_INFO("specification version: ", ONECCL_SPEC_VERSION);
#ifdef CCL_ENABLE_SYCL
        LOG_INFO("compute backend: ", version.cl_backend_name);
#endif // CCL_ENABLE_SYCL

#ifdef ENABLE_DEBUG
        const char* build_mode = "debug";
#else // ENABLE_DEBUG
        const char* build_mode = "release";
#endif // ENABLE_DEBUG
        LOG_INFO("build mode: ", build_mode);
        LOG_INFO("C compiler: ", CCL_C_COMPILER);
        LOG_INFO("C++ compiler: ", CCL_CXX_COMPILER);
        LOG_INFO(global_data.hwloc_wrapper->to_string());
    }

    auto local_proc_idx = global_data.get_local_proc_idx();
    auto local_proc_count = global_data.get_local_proc_count();

    if (rank < local_proc_count) {
        for (size_t w_idx = 0; w_idx < worker_count; w_idx++) {
            LOG_INFO("local process [",
                     local_proc_idx,
                     ":",
                     local_proc_count,
                     "]: worker: ",
                     w_idx,
                     ", cpu: ",
                     worker_affinity[local_proc_idx * worker_count + w_idx],
                     ", numa: ",
                     worker_mem_affinity[local_proc_idx * worker_count + w_idx]);
        }
    }

    if (rank != 0)
        return;

    LOG_INFO(CCL_WORKER_COUNT, ": ", worker_count);
    LOG_INFO(CCL_WORKER_OFFLOAD, ": ", worker_offload);
    LOG_INFO(CCL_WORKER_WAIT, ": ", worker_wait);

    LOG_INFO(CCL_LOG_LEVEL, ": ", str_by_enum(ccl_logger::level_names, log_level));
    LOG_INFO(CCL_ABORT_ON_THROW, ": ", abort_on_throw);
    LOG_INFO(CCL_QUEUE_DUMP, ": ", queue_dump);
    LOG_INFO(CCL_SCHED_DUMP, ": ", sched_dump);
    LOG_INFO(CCL_SCHED_PROFILE, ": ", sched_profile);
    LOG_INFO(CCL_ENTRY_MAX_UPDATE_TIME_SEC,
             ": ",
             (entry_max_update_time_sec != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(entry_max_update_time_sec)
                 : CCL_ENV_STR_NOT_SPECIFIED);

    LOG_INFO(CCL_FRAMEWORK, ": ", str_by_enum(ccl_framework_type_names, fw_type));

    LOG_INFO(CCL_ATL_TRANSPORT, ": ", str_by_enum(atl_transport_names, atl_transport));
    LOG_INFO(CCL_ATL_SHM, ": ", enable_shm);
    LOG_INFO(CCL_ATL_RMA, ": ", enable_rma);
    LOG_INFO(CCL_ATL_HMEM, ": ", enable_hmem);
    LOG_INFO(CCL_ATL_SEND_PROXY, ": ", str_by_enum(atl_send_proxy_names, atl_send_proxy));
    LOG_INFO(CCL_ATL_CACHE, ": ", enable_atl_cache);
    LOG_DEBUG(CCL_ATL_SYNC_COLL, ": ", enable_sync_coll);
    LOG_DEBUG(CCL_ATL_EXTRA_EP, ": ", enable_extra_ep);

    LOG_INFO(CCL_MNIC, ": ", str_by_enum(mnic_type_names, mnic_type));
    LOG_INFO(
        CCL_MNIC_NAME, ": ", (mnic_name_raw.length()) ? mnic_name_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_MNIC_COUNT, ": ", mnic_count);
    LOG_INFO(CCL_MNIC_OFFSET, ": ", str_by_enum(mnic_offset_names, mnic_offset));

    LOG_INFO(CCL_ALGO_FALLBACK, ": ", enable_algo_fallback);
    LOG_INFO(CCL_ALLGATHERV,
             ": ",
             (allgatherv_algo_raw.length()) ? allgatherv_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ALLREDUCE,
             ": ",
             (allreduce_algo_raw.length()) ? allreduce_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ALLTOALL,
             ": ",
             (alltoall_algo_raw.length()) ? alltoall_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ALLTOALLV,
             ": ",
             (alltoallv_algo_raw.length()) ? alltoallv_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_BARRIER,
             ": ",
             (barrier_algo_raw.length()) ? barrier_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(
        CCL_BCAST, ": ", (bcast_algo_raw.length()) ? bcast_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(
        CCL_REDUCE, ": ", (reduce_algo_raw.length()) ? reduce_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(
        CCL_REDUCE_SCATTER,
        ": ",
        (reduce_scatter_algo_raw.length()) ? reduce_scatter_algo_raw : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_UNORDERED_COLL, ": ", enable_unordered_coll);

    LOG_INFO(CCL_FUSION, ": ", enable_fusion);
    LOG_INFO(CCL_FUSION_BYTES_THRESHOLD, ": ", fusion_bytes_threshold);
    LOG_INFO(CCL_FUSION_COUNT_THRESHOLD, ": ", fusion_count_threshold);
    LOG_INFO(CCL_FUSION_CHECK_URGENT, ": ", fusion_check_urgent);
    LOG_INFO(CCL_FUSION_CYCLE_MS, ": ", fusion_cycle_ms);

    LOG_INFO(CCL_PRIORITY, ": ", str_by_enum(priority_mode_names, priority_mode));
    LOG_INFO(CCL_SPIN_COUNT, ": ", spin_count);
    LOG_INFO(CCL_YIELD, ": ", str_by_enum(ccl_yield_type_names, yield_type));
    LOG_INFO(CCL_MAX_SHORT_SIZE, ": ", max_short_size);
    LOG_INFO(CCL_BCAST_PART_COUNT,
             ": ",
             (bcast_part_count != CCL_ENV_SIZET_NOT_SPECIFIED) ? std::to_string(bcast_part_count)
                                                               : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_CACHE_KEY, ": ", str_by_enum(ccl_sched_key::key_type_names, cache_key_type));
    LOG_INFO(CCL_CACHE_FLUSH, ": ", enable_cache_flush);
    LOG_INFO(CCL_BUFFER_CACHE, ": ", enable_buffer_cache);
    LOG_INFO(CCL_STRICT_ORDER, ": ", enable_strict_order);
    LOG_INFO(CCL_STAGING_BUFFER, ": ", str_by_enum(staging_buffer_names, staging_buffer));
    LOG_INFO(CCL_OP_SYNC, ": ", enable_op_sync);
    LOG_INFO(CCL_USE_EXTERNAL_QUEUE, ": ", enable_external_queue);

    LOG_INFO(CCL_CHUNK_COUNT, ": ", chunk_count);
    LOG_INFO(CCL_MIN_CHUNK_SIZE, ": ", min_chunk_size);
    LOG_INFO(CCL_RS_CHUNK_COUNT, ": ", rs_chunk_count);
    LOG_INFO(CCL_RS_MIN_CHUNK_SIZE, ": ", rs_min_chunk_size);

#ifdef CCL_ENABLE_SYCL
    LOG_INFO(CCL_ALLGATHERV_TOPO_LARGE_SCALE, ": ", allgatherv_topo_large_scale);
#endif // CCL_ENABLE_SYCL

    LOG_INFO(CCL_ALLREDUCE_NREDUCE_BUFFERING, ": ", allreduce_nreduce_buffering);
    LOG_INFO(CCL_ALLREDUCE_NREDUCE_SEGMENT_SIZE,
             ": ",
             (allreduce_nreduce_segment_size != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(allreduce_nreduce_segment_size)
                 : CCL_ENV_STR_NOT_SPECIFIED);

    LOG_INFO(CCL_ALLREDUCE_2D_CHUNK_COUNT, ": ", allreduce_2d_chunk_count);
    LOG_INFO(CCL_ALLREDUCE_2D_MIN_CHUNK_SIZE, ": ", allreduce_2d_min_chunk_size);
    LOG_INFO(CCL_ALLREDUCE_2D_SWITCH_DIMS, ": ", allreduce_2d_switch_dims);

    LOG_INFO(CCL_ALLTOALL_SCATTER_MAX_OPS,
             ": ",
             (alltoall_scatter_max_ops != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(alltoall_scatter_max_ops)
                 : CCL_ENV_STR_NOT_SPECIFIED);

    LOG_INFO(CCL_BACKEND, ": ", str_by_enum(backend_names, backend));

    LOG_INFO(CCL_LOCAL_RANK,
             ": ",
             (local_rank != CCL_ENV_INT_NOT_SPECIFIED) ? std::to_string(local_rank)
                                                       : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_LOCAL_SIZE,
             ": ",
             (local_size != CCL_ENV_INT_NOT_SPECIFIED) ? std::to_string(local_size)
                                                       : CCL_ENV_STR_NOT_SPECIFIED);

    LOG_INFO(CCL_PROCESS_LAUNCHER, ": ", str_by_enum(process_launcher_names, process_launcher));

#ifdef CCL_ENABLE_MPI
    LOG_INFO(CCL_MPI_LIBRARY_PATH,
             ": ",
             (!mpi_lib_path.empty()) ? mpi_lib_path : CCL_ENV_STR_NOT_SPECIFIED);
#endif // CCL_ENABLE_MPI
    LOG_INFO(CCL_OFI_LIBRARY_PATH,
             ": ",
             (!ofi_lib_path.empty()) ? ofi_lib_path : CCL_ENV_STR_NOT_SPECIFIED);

#ifdef CCL_ENABLE_SYCL
    LOG_INFO(CCL_TOPO_ALGO, ": ", enable_topo_algo);
    LOG_INFO(CCL_TOPO_COLOR, ": ", str_by_enum(topo_color_names, topo_color));
    LOG_INFO(CCL_TOPO_P2P_ACCESS,
             ": ",
             (enable_p2p_access != CCL_ENV_INT_NOT_SPECIFIED) ? std::to_string(enable_p2p_access)
                                                              : CCL_ENV_STR_NOT_SPECIFIED);

    LOG_INFO(
        CCL_KERNEL_PATH, ": ", (!kernel_path.empty()) ? kernel_path : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_KERNEL_DEBUG, ": ", kernel_debug);
    LOG_INFO(CCL_KERNEL_GROUP_SIZE, ": ", kernel_group_size);
    LOG_INFO(CCL_KERNEL_GROUP_COUNT,
             ": ",
             (kernel_group_count != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(kernel_group_count)
                 : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_KERNEL_SYNC, ": ", enable_kernel_sync);
    LOG_INFO(CCL_KERNEL_1S_LEAD, ": ", kernel_1s_lead);
    LOG_INFO(CCL_KERNEL_1S_USE_COPY_OPS, ": ", enable_kernel_1s_copy_ops);
    LOG_INFO(CCL_KERNEL_1S_IPC_WA, ": ", enable_kernel_1s_ipc_wa);
    LOG_INFO(CCL_KERNEL_CLOSE_FD_WA, ": ", enable_close_fd_wa);

    LOG_INFO(CCL_SYCL_OUTPUT_EVENT, ": ", enable_sycl_output_event);
    LOG_INFO(CCL_USE_HMEM, ": ", use_hmem);

    LOG_INFO(CCL_ZE_BARRIER, ": ", enable_ze_barrier);
    LOG_INFO(CCL_ZE_BIDIR_ALGO, ": ", enable_ze_bidir_algo);
    LOG_INFO(CCL_ZE_CACHE, ": ", enable_ze_cache);
    LOG_INFO(CCL_ZE_CACHE_IPC_HANDLES, ": ", enable_ze_cache_ipc_handles);
    LOG_INFO(CCL_ZE_CACHE_IPC_HANDLES_THRESHOLD, ": ", ze_cache_ipc_handles_threshold);
    LOG_INFO(CCL_ZE_SINGLE_LIST, ": ", enable_ze_single_list);
    LOG_INFO(CCL_ZE_DISABLE_FAMILY_CHECK, ": ", disable_ze_family_check);
    LOG_INFO(CCL_ZE_DISABLE_PORT_CHECK, ": ", disable_ze_port_check);
    LOG_INFO(CCL_ZE_SERIALIZE, ": ", ze_serialize_mode);
    LOG_INFO(CCL_ZE_COPY_ENGINE, ": ", str_by_enum(ccl::ze::copy_engine_names, ze_copy_engine));
    LOG_INFO(CCL_ZE_MAX_COMPUTE_QUEUES,
             ": ",
             (ze_max_compute_queues != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(ze_max_compute_queues)
                 : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ZE_MAX_COPY_QUEUES,
             ": ",
             (ze_max_copy_queues != CCL_ENV_SIZET_NOT_SPECIFIED)
                 ? std::to_string(ze_max_copy_queues)
                 : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ZE_ENABLE_CCS_FALLBACK_FOR_COPY, ": ", ze_enable_ccs_fallback_for_copy);
    LOG_INFO(CCL_ZE_LIST_DUMP, ": ", enable_ze_list_dump);
    LOG_INFO(CCL_ZE_QUEUE_INDEX_OFFSET, ": ", ze_queue_index_offset);
    LOG_INFO(CCL_ZE_CLOSE_IPC_WA, ": ", ze_close_ipc_wa);
    LOG_INFO(CCL_ZE_LIBRARY_PATH,
             ": ",
             (!ze_lib_path.empty()) ? ze_lib_path : CCL_ENV_STR_NOT_SPECIFIED);
    LOG_INFO(CCL_ZE_ENABLE, ": ", ze_enable);
    LOG_INFO(CCL_ZE_FINI_WA, ": ", ze_fini_wa);
    LOG_INFO(CCL_ZE_MULTI_WORKERS, ": ", ze_multi_workers);
    LOG_INFO(CCL_ZE_IPC_EXCHANGE, ": ", str_by_enum(ze::ipc_exchange_names, ze_ipc_exchange));
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_ITT
    LOG_INFO(CCL_ITT_LEVEL, ": ", itt_level);
#endif // CCL_ENABLE_ITT

    LOG_INFO(CCL_BF16, ": ", str_by_enum(bf16_impl_names, bf16_impl_type));
    LOG_INFO(CCL_FP16, ": ", str_by_enum(fp16_impl_names, fp16_impl_type));

    char* ccl_root = getenv("CCL_ROOT");
    LOG_INFO("CCL_ROOT: ", (ccl_root) ? ccl_root : CCL_ENV_STR_NOT_SPECIFIED);

    char* impi_root = getenv("I_MPI_ROOT");
    LOG_INFO("I_MPI_ROOT: ", (impi_root) ? impi_root : CCL_ENV_STR_NOT_SPECIFIED);

    char* fi_provider_path = getenv("FI_PROVIDER_PATH");
    LOG_INFO("FI_PROVIDER_PATH: ",
             (fi_provider_path) ? fi_provider_path : CCL_ENV_STR_NOT_SPECIFIED);

    char* fi_provider = getenv("FI_PROVIDER");
    LOG_INFO("FI_PROVIDER: ", (fi_provider) ? fi_provider : CCL_ENV_STR_NOT_SPECIFIED);

    global_data.algorithm_selector->print();
}

void env_data::set_internal_env() {
    auto attr = ccl_executor::generate_atl_attr(*this);
    atl_comm_manager::set_internal_env(attr);
    if (log_level >= ccl_log_level::info) {
        setenv("I_MPI_DEBUG", "4", 0);
    }
}

int env_data::env_2_worker_affinity_auto(int local_proc_idx, size_t workers_per_process) {
    char* available_cores = std::getenv(I_MPI_AVAILABLE_CORES_ENV);
    CCL_THROW_IF_NOT(available_cores && strlen(available_cores) != 0,
                     "auto pinning requires ",
                     I_MPI_AVAILABLE_CORES_ENV,
                     " env variable to be set");

    LOG_DEBUG("available_cores ", available_cores);

    std::vector<size_t> cores;
    ccl::utils::str_to_array(available_cores, std::string(I_MPI_AVAILABLE_CORES_DELIMS), cores);

    CCL_THROW_IF_NOT(workers_per_process <= cores.size(),
                     "failed to implicitly set workers affinity, "
                     "the number of workers (",
                     workers_per_process,
                     ") exceeds the number of available cores per process (",
                     cores.size(),
                     "), consider increasing the number of cores per process ",
                     "or explicitly setting of workers affinity using ",
                     CCL_WORKER_AFFINITY);

    if ((workers_per_process == cores.size()) && worker_offload) {
        LOG_WARN("the number of workers (",
                 workers_per_process,
                 ") matches the number of available cores per process,"
                 " this may lead to contention between workers and"
                 " application threads");
        if (!std::getenv(CCL_WORKER_OFFLOAD)) {
            worker_offload = 0;
            LOG_WARN("workers are disabled,"
                     " to forcibly enable them set ",
                     CCL_WORKER_OFFLOAD,
                     "=1");
        }
        else {
            LOG_WARN("consider increasing the number of cores per process",
                     " or disabling workers using ",
                     CCL_WORKER_OFFLOAD,
                     "=0");
        }
    }

    size_t worker_cores_start = cores.size() - workers_per_process;
    for (size_t idx = 0; idx < workers_per_process; ++idx) {
        worker_affinity[local_proc_idx * workers_per_process + idx] =
            cores[worker_cores_start + idx];
    }
    return 1;
}

int env_data::parse_number(const std::string& number_str, size_t& result) {
    char* end_ptr;
    const char* number_str_ptr = number_str.c_str();

    errno = 0;
    auto core_id = std::strtol(number_str_ptr, &end_ptr, 10);

    if ((errno == ERANGE && (core_id == LONG_MAX || core_id == LONG_MIN)) ||
        (errno != 0 && core_id == 0)) {
        LOG_ERROR("core id value is invalid in string: ", number_str);
        return 0;
    }
    if (end_ptr == number_str_ptr) {
        LOG_ERROR("no digits were found in string: ", number_str);
        return 0;
    }
    if (core_id < 0) {
        LOG_ERROR("core id cannot be less than zero but got ", core_id, " in string: ", number_str);
        return 0;
    }
    result = core_id;
    return 1;
}

int env_data::parse_affinity(const std::string& input,
                             std::vector<ssize_t>& output,
                             size_t expected_output_size) {
    size_t idx;
    char* range_str;

    /* create copy of input string because it will be modified in strsep */
    std::string input_copy(input.c_str());
    char* input_str = (char*)input_copy.c_str();

    output.clear();

    while (input_str) {
        range_str = strsep(&input_str, ",");
        if (!range_str) {
            break;
        }

        auto range = ccl::utils::tokenize<std::vector<std::string>>(std::string(range_str), '-');

        if ((range.size() != 2) && (range.size() != 1)) {
            LOG_ERROR(
                "unexpected format in input: ",
                input,
                ", specify range values using <first_value>-<last_value> or single value using <value>");
            return 0;
        }

        if (range.size() == 1) {
            /* to unify logic below */
            range.push_back(*range.begin());
        }

        CCL_ASSERT(range.size() == 2, "unexpected number of values in range");

        size_t first_value, last_value;
        if (!parse_number(range[0], first_value) || !parse_number(range[1], last_value)) {
            return 0;
        }

        if (first_value > last_value) {
            LOG_ERROR("unexpected first and last values in range: ",
                      range_str,
                      ", first value should be less or equal to last value");
            return 0;
        }

        for (idx = first_value; idx <= last_value; idx++) {
            output.push_back(idx);
        }
    }

    if (output.size() < expected_output_size) {
        LOG_ERROR("unexpected number of values in input: ",
                  input,
                  ", expected at least ",
                  expected_output_size,
                  " values");
        return 0;
    }

    return 1;
}

int env_data::env_2_worker_affinity(int local_proc_idx, int local_proc_count) {
    CCL_THROW_IF_NOT(local_proc_count > 0);

    size_t idx;
    char* env_to_parse = getenv(CCL_WORKER_AFFINITY);
    size_t system_core_count;
    size_t affinity_size = local_proc_count * worker_count;

    if (!env_to_parse || (strlen(env_to_parse) == 0) || (strcmp(env_to_parse, "auto") == 0)) {
        worker_affinity.assign(affinity_size, CCL_UNDEFINED_CPU_ID);
        if (std::getenv(I_MPI_AVAILABLE_CORES_ENV)) {
            /* generate auto affinity based on IMPI process pinning */
            return env_2_worker_affinity_auto(local_proc_idx, worker_count);
        }
        else {
            /* generate auto affinity as last N cores */
            system_core_count = sysconf(_SC_NPROCESSORS_ONLN);
            for (idx = 0; idx < affinity_size; idx++) {
                if (idx < system_core_count) {
                    worker_affinity[idx] = system_core_count - idx - 1;
                }
                else {
                    worker_affinity[idx] = worker_affinity[idx % system_core_count];
                }
            }
            return 1;
        }
    }

    CCL_THROW_IF_NOT(parse_affinity(env_to_parse, worker_affinity, affinity_size),
                     "failed to parse worker affinity");

    return 1;
}

int env_data::env_2_worker_mem_affinity(int local_proc_count) {
    CCL_THROW_IF_NOT(worker_affinity.size() > 0);
    CCL_THROW_IF_NOT(local_proc_count > 0);

    size_t idx;
    char* env_to_parse = getenv(CCL_WORKER_MEM_AFFINITY);
    size_t affinity_size = local_proc_count * worker_count;
    CCL_THROW_IF_NOT(affinity_size <= worker_affinity.size());

    if (!env_to_parse || (strlen(env_to_parse) == 0) || (strcmp(env_to_parse, "auto") == 0)) {
        worker_mem_affinity.assign(affinity_size, CCL_UNDEFINED_NUMA_NODE);
        /* generate list of default numa nodes, local wrt worker cores */
        for (idx = 0; idx < affinity_size; idx++) {
            worker_mem_affinity[idx] =
                ccl::global_data::get().hwloc_wrapper->get_numa_node_by_cpu(worker_affinity[idx]);
        }
        return 1;
    }

    CCL_THROW_IF_NOT(parse_affinity(env_to_parse, worker_mem_affinity, affinity_size),
                     "failed to parse worker memory affinity");

    return 1;
}

void env_data::env_2_atl_transport() {
#ifdef CCL_ENABLE_MPI
    if (!getenv(CCL_ATL_TRANSPORT) && !with_mpirun()) {
        LOG_WARN("did not find MPI-launcher specific variables, switch to ATL/OFI, "
                 "to force enable ATL/MPI set CCL_ATL_TRANSPORT=mpi");

        atl_transport = ccl_atl_ofi;
    }
    else
#endif // CCL_ENABLE_MPI
        env_2_enum(CCL_ATL_TRANSPORT, atl_transport_names, atl_transport);
}

bool env_data::with_mpirun() {
    return (getenv("MPI_LOCALRANKID") || getenv("MPI_LOCALNRANKS") || getenv("PMI_RANK") ||
            getenv("PMI_SIZE"))
               ? true
               : false;
}

} // namespace ccl
