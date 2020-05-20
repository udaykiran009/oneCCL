#pragma once

#include <initializer_list>
#include <map>
#include <memory>
#include <list>
#include <set>
#include <vector>

#include "common/comm/l0/devices/ccl_gpu_base_comm.hpp"

namespace native
{
class ccl_ipc_gpu_comm : public ccl_gpu_base_comm<ccl_ipc_gpu_comm,
                                                  gpu_types::IPC_DESTINATION_GPU>,
                         public module_loader<ccl_ipc_gpu_comm>
{
public:
    using base = ccl_gpu_base_comm<ccl_ipc_gpu_comm,
                                   gpu_types::IPC_DESTINATION_GPU>;
    using base::comm_rank_t;

    template<ccl_coll_type algo_type,
             ccl::device_topology_type topology_type>
    using gpu_module_t = ipc_gpu_coll_module<algo_type, topology_type>;

    template<ccl_coll_type algo_type,
             ccl::device_topology_type topology_type,
             class native_data_type>
    using gpu_kernel_t = typename gpu_module_t<algo_type, topology_type>::template kernel<native_data_type>;

    using supported_modules = supported_device_modules<ipc_gpu_coll_module>;

    static constexpr const char* name_impl()
    {
        return "DESTINATION_IPC_GPU";
    }

    ccl_ipc_gpu_comm(ccl_device& assigned_device, comm_rank_t idx, size_t size,
                     ccl::device_topology_type topology_type);
    ~ccl_ipc_gpu_comm() = default;

    std::string to_string_impl() const;

    template<ccl_coll_type module_type,
             ccl::device_topology_type topology_type,
             class native_data_type>
    gpu_kernel_t<module_type, topology_type, native_data_type>& get_gpu_kernel()
    {
        auto& ptr = base::template get_gpu_module_unsafe<module_type, topology_type>(registered_modules);
        assert(ptr);
        if (not std::is_same<native_data_type, float>::value)
        {
            throw std::runtime_error(std::string(__PRETTY_FUNCTION__) + "Only float is supported");
        }
        return ptr->template get_main_function<native_data_type>();
    }

    template<ccl_coll_type module_type,
             ccl::device_topology_type topology_type>
    std::string create_module_impl(const ze_module_desc_t &module_data)
    {
        std::get<topology_type>(std::get<module_type>(registered_modules)).reset(new gpu_module_t<module_type, topology_type>(nullptr));
        return {"IPC module storage"};
    }

private:
    supported_modules registered_modules;
};

}
