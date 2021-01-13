#pragma once
#include <map>
#include <memory>
#include <list>
#include <set>
#include <vector>

#include "common/comm/l0/devices/ccl_gpu_base_comm.hpp"
#include "common/comm/l0/devices/proxy_observer_types.hpp"
#include "common/comm/l0/context/scaling_ctx/observer_session_key.hpp"

namespace native {

//Adapter for different thread devices
template <class device_t>
class ccl_numa_proxy
        : public ccl_gpu_base_comm<ccl_numa_proxy<device_t>,
                                   gpu_types::NUMA_PROXY_GPU_TYPES + device_t::type_idx()>,
          public proxy_observer_specific<ccl_numa_proxy<device_t>> {
public:
    using base = ccl_gpu_base_comm<ccl_numa_proxy<device_t>,
                                   gpu_types::NUMA_PROXY_GPU_TYPES + device_t::type_idx()>;
    using typename base::comm_rank_t;
    using impl_t = device_t;
    using proxy_base = proxy_observer_specific<ccl_numa_proxy<device_t>>;

    template <ccl_coll_type algo_type, ccl::group_split_type group, ccl::device_topology_type mode>
    using gpu_module_t =
        typename device_t::template gpu_module_t<algo_type, group, mode>; //same as in-process GPU

    template <ccl_coll_type algo_type,
              ccl::group_split_type group,
              ccl::device_topology_type mode,
              class native_data_type>
    using gpu_kernel_t =
        typename gpu_module_t<algo_type, group, mode>::template kernel_numa<native_data_type>;

    static constexpr const char* name_impl() {
        return "NUMA_PROXY";
    }

    ccl_numa_proxy(ccl_device& assigned_device,
                   typename base::comm_rank_t idx,
                   device_t& process_device)
            : base(assigned_device, idx),
              wrapped_gpu_comm(process_device) {}

    ~ccl_numa_proxy() = default;

    std::string to_string_impl() const {
        std::string ret(name_impl());
        ret = ret + "(" + wrapped_gpu_comm.to_string_impl() + ")";
        return ret;
    }

    template <ccl_coll_type module_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class native_data_type>
    gpu_kernel_t<module_type, group_id, class_id, native_data_type>& get_gpu_kernel() {
        auto& ptr = wrapped_gpu_comm.template get_gpu_module<module_type, group_id, class_id>();
        return ptr.template get_numa_main_function<native_data_type>();
    }

    template <class native_data_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class gpu_entry>
    gpu_kernel_t<gpu_entry::type(), group_id, class_id, native_data_type>& register_entry(
        gpu_entry& entry) {
        static_assert(group_id == ccl::group_split_type::cluster,
                      "ccl_numa_proxy available for ccl::group_split_type::cluster only");

        const topology_addr<group_id, class_id>& comm_addr =
            base::template get_comm_data<group_id, class_id>();
        LOG_DEBUG("entry: ", gpu_entry::class_name(), " registered on: ", comm_addr.to_string());

        using kernel_func_type =
            gpu_kernel_t<gpu_entry::type(), group_id, class_id, native_data_type>;
        kernel_func_type& main_func =
            get_gpu_kernel<gpu_entry::type(), group_id, class_id, native_data_type>();
        main_func.set_rank(comm_addr.rank);
        main_func.set_size(comm_addr.size);

        // alloc shared data structure to notify host side with device parital result
        observer::invoke_params<gpu_entry::type(), native_data_type> params = entry.get_numa_data();

        // invoke host-side context creation
        this->template invoke<group_id, class_id>(entry.get_numa_session_key(), params);

        // bind shared data to kernel
        const auto& out_ctx_params = params.get_ctx_params();
        main_func.template set_arg<typename kernel_func_type::event_prod_chunk_mem_arg>(
            out_ctx_params.numa_staged_memory->get());
        main_func.template set_arg<typename kernel_func_type::event_prod_bytes_arg>(
            out_ctx_params.staged_memory_size_counter->get());

        main_func.template set_arg<typename kernel_func_type::event_consumed_bytes_offset_arg>(
            out_ctx_params.producer_aggregated_memory_offset->get());
        main_func.template set_arg<typename kernel_func_type::event_consumed_chunk_mem_arg>(
            out_ctx_params.total_producers_aggregated_memory->get());
        main_func.template set_arg<typename kernel_func_type::event_consumed_bytes_arg>(
            out_ctx_params.total_producers_aggregated_size_counter->get());

        return main_func;
    }

private:
    device_t& wrapped_gpu_comm;
};
} // namespace native
