#pragma once
#include "native_device_api/export_api.hpp"
#include "ccl_type_traits.hpp"
#include "common/comm/l0/modules/kernel_functions.hpp"

namespace native
{

//module, collection of functions
struct gpu_module_base
{
    using handle = ze_module_handle_t;
    using kernel_handle = ze_kernel_handle_t;
    using imported_kernels = std::map<std::string, kernel_handle>;

    gpu_module_base(handle module_handle);
    ~gpu_module_base();

    handle get() const;
    void release();
    kernel_handle import_kernel(const std::string& name);

    handle module;
    imported_kernels functions;
};

//specific type module implementations:
//1) in-process gpu module
template<ccl_coll_type type, ccl::device_topology_type topology>
struct gpu_coll_module : private gpu_module_base
{
    static constexpr ccl_coll_type get_coll_type() { return type; }
    static constexpr ccl::device_topology_type get_topology_type() { return topology; }

    gpu_coll_module(handle module_handle) :
     gpu_module_base(module_handle)
    {
    }
};

//2) out-of-process gpu module
template<ccl_coll_type type, ccl::device_topology_type topology>
struct ipc_gpu_coll_module : private gpu_module_base
{
    static constexpr ccl_coll_type get_coll_type() { return type; }
    static constexpr ccl::device_topology_type get_topology_type() { return topology; }

    ipc_gpu_coll_module(handle module_handle) :
     gpu_module_base(module_handle)
    {
    }
};

//3) virtual gpu module
template<ccl_coll_type type, ccl::device_topology_type topology>
struct virtual_gpu_coll_module
{
    static constexpr ccl_coll_type get_coll_type() { return type; }
    static constexpr ccl::device_topology_type get_topology_type() { return topology; }

    virtual_gpu_coll_module(std::shared_ptr<gpu_coll_module<type, topology>> real_module) :
     real_module_ref(real_module)
    {
    }
    std::shared_ptr<gpu_coll_module<type, topology>> real_module_ref;
};


template<ccl_coll_type type, ccl::device_topology_type topology_type>
struct coll_module_traits
{
    static constexpr ccl_coll_type coll_type() {return type;}
    static constexpr ccl::device_topology_type topology() {return topology_type;}
    static constexpr ccl::device_topology_class topology_class() { return ccl::topology_to_class<topology_type>(); }
};
}
