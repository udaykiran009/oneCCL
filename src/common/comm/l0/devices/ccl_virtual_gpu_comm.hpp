#pragma once

#include "common/comm/l0/devices/ccl_gpu_comm.hpp"

namespace native {

class ccl_virtual_gpu_comm : public ccl_gpu_base_comm<ccl_virtual_gpu_comm, gpu_types::VIRTUAL_GPU>,
                             public module_loader<ccl_virtual_gpu_comm> {
public:
    using base = ccl_gpu_base_comm<ccl_virtual_gpu_comm, gpu_types::VIRTUAL_GPU>;
    using base::comm_rank_t;

    using impl_t = ccl_virtual_gpu_comm;

    template <ccl_coll_type algo_type, ccl::group_split_type group, ccl::device_topology_type mode>
    using gpu_module_t = virtual_device_coll_module<algo_type, group, mode>;

    template <ccl_coll_type algo_type, ccl::group_split_type group, ccl::device_topology_type mode>
    using kernel_class_t = typename gpu_module_t<algo_type, group, mode>::main_class;

    template <ccl_coll_type algo_type,
              ccl::group_split_type group,
              ccl::device_topology_type mode,
              class kernel_params>
    using gpu_kernel_t =
        typename kernel_class_t<algo_type, group, mode>::template kernel_t<kernel_params>;

    using supported_modules = supported_device_modules<gpu_module_t>;

    static constexpr const char* name_impl() {
        return "VIRTUAL_GPU";
    }

    std::string to_string_impl() const;

    ccl_virtual_gpu_comm(ccl_device& device, comm_rank_t idx, ccl_gpu_comm& real_gpu);
    ~ccl_virtual_gpu_comm() = default;

    template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
    topology_addr<group_id, class_id> get_real_comm_data() const {
        return real_gpu_comm.get_comm_data<group_id, class_id>();
    }

    template <ccl_coll_type module_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id>
    gpu_module_t<module_type, group_id, class_id>& get_gpu_module() {
        auto& ptr =
            base::template get_gpu_module_unsafe<module_type, group_id, class_id, gpu_module_t>(
                registered_modules);
        assert(ptr);
        return *ptr;
    }

    template <ccl_coll_type module_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class kernel_params>
    gpu_kernel_t<module_type, group_id, class_id, kernel_params>& get_gpu_kernel() {
        auto& ptr = get_gpu_module<module_type, group_id, class_id>();

        using requested_class = kernel_class_t<module_type, group_id, class_id>;
        return ptr.template get_class<requested_class>().template get<kernel_params>();
    }

    template <class kernel_params,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id,
              class gpu_entry>
    gpu_kernel_t<gpu_entry::type(), group_id, class_id, kernel_params>& register_entry(
        gpu_entry& entry) {
        const topology_addr<group_id, class_id>& comm_addr = get_comm_data<group_id, class_id>();
        LOG_DEBUG("entry: ", gpu_entry::class_name(), " registered on: ", comm_addr.to_string());

        auto& main_func = get_gpu_kernel<gpu_entry::type(), group_id, class_id, kernel_params>();
        main_func.set_rank(comm_addr.rank);
        main_func.set_size(comm_addr.size);
        return main_func;
    }

    template <ccl_coll_type module_type,
              ccl::group_split_type group_id,
              ccl::device_topology_type class_id>
    std::string create_module_impl(const ze_module_desc_t& module_data) {
        //virtual based on real
        auto real_kernel = real_gpu_comm.get_gpu_module_ptr<module_type, group_id, class_id>();

        std::get<::utils::enum_to_underlying(class_id)>(
            std::get<::utils::enum_to_underlying(group_id)>(
                std::get<module_type>(registered_modules)))
            .reset(new gpu_module_t<module_type, group_id, class_id>(real_kernel));
        return { "virtual module" };
    }

    cmd_list_proxy get_cmd_list(std::shared_ptr<ccl_context> ctx,
                                const ze_command_list_desc_t& properties);

    fence_proxy get_fence(const ccl_device::device_queue& cmd_queue,
                          std::shared_ptr<ccl_context> ctx);

private:
    ccl_gpu_comm& real_gpu_comm;
    supported_modules registered_modules;
};
} // namespace native
