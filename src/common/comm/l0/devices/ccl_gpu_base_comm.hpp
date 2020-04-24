#pragma once

#include <map>
#include <memory>
#include <list>
#include <set>
#include <vector>

#include "coll/algorithms/algorithms_enum.hpp"
#include "native_device_api/export_api.hpp"

#include "common/comm/l0/device_group_routing_schema.hpp"
#include "common/comm/l0/gpu_device_types.hpp"
#include "common/comm/l0/modules/ring/allreduce_entry_module.hpp"
#include "common/comm/l0/modules/a2a/allreduce_module.hpp"
#include "common/comm/l0/modules/supported_modules.hpp"

#include "common/comm/l0/modules/modules_source_data.hpp"
#include "common/comm/l0/gpu_comm_utils.hpp"

namespace native
{

template<class gpu_impl, gpu_types type>
class ccl_gpu_base_comm
{
public:
    using comm_rank_t = size_t;
    using type_idx_t = typename std::underlying_type<gpu_types>::type;
    ccl_gpu_base_comm(ccl_device& assigned_device, comm_rank_t idx) :
        index_in_group(idx),
        device(assigned_device)

    {
    }

    ~ccl_gpu_base_comm() = default;

    gpu_impl* get_this()
    {
        return static_cast<gpu_impl*>(this);
    }

    const gpu_impl* get_this() const
    {
        return static_cast<const gpu_impl*>(this);
    }

    static constexpr const char* name()
    {
        return gpu_impl::name_impl();
    }

    std::string to_string() const
    {
        return get_this()->to_string_impl();
    }

    static constexpr type_idx_t type_idx()
    {
        return static_cast<type_idx_t>(type);
    }

    ccl_device& get_device()
    {
        return device;
    }
    [[deprecated]]
    comm_rank_t get_index_in_group() const
    {
        return index_in_group;
    }

    template<ccl::device_topology_type topology_type>
    bool reset_rank(comm_rank_t new_rank, comm_rank_t new_size)
    {
        rank = new_rank;
        size = new_size;
        return device_routing_web.insert<topology_type>(new_rank, new_size);   //consider inheritance
    }

    template<ccl::device_topology_type topology_type>
    const topology_addr<topology_type>& get_comm_data() const
    {
        return device_routing_web.get<topology_type>();
    }

    template<ccl::device_topology_type topology_type>
    std::string comm_to_str() const
    {
        return device_routing_web.to_string<topology_type>();
    }

    std::string comm_to_str() const
    {
        return device_routing_web.to_string();
    }

    template<ccl_coll_type module_type,
             ccl::device_topology_type topology_type,
             template<ccl_coll_type, ccl::device_topology_type> class module_impl>
    static std::shared_ptr<module_impl<module_type, topology_type>>& get_gpu_module_unsafe(supported_device_modules<module_impl> &modules)
    {
        return std::get<topology_type>(std::get<module_type>(modules));
    }
protected:
    size_t index_in_group;

    aggregated_topology_addr device_routing_web;
    ccl_device& device;

    mutable size_t rank;//TODO
    mutable size_t size;//TODO
};

}
