#pragma once
#include <memory>
#include "common/comm/l0/modules/base_entry_module.hpp"

namespace native
{

template<ccl_coll_type type,
         template<typename> class kernel_function_impl>
struct real_gpu_typed_module : private gpu_module_base
{
    template<class native_data_type>
    using kernel = kernel_function_impl<native_data_type>;
    using handle = gpu_module_base::handle;

    real_gpu_typed_module(handle module_handle):
        gpu_module_base(module_handle)
    {
        LOG_DEBUG("Real gpu module created:", ccl_coll_type_to_str(type));
        main_function_args.handle = import_kernel(kernel<float>::name());
        main_function_args_int.handle = import_kernel(std::string(kernel<int>::name())+ "_int");//TODO just example
        LOG_DEBUG("Imported functions count: ", functions.size());
    }

    handle get() const
    {
        return module;
    }

    //TODO - use std::tuple  for multiple types support
    kernel<float> main_function_args;
    kernel<int> main_function_args_int;

protected:
    ~real_gpu_typed_module() = default;
};


template<ccl_coll_type type,
         template<typename> class kernel_function_impl>
struct ipc_gpu_typed_module : private gpu_module_base
{
    template<class native_data_type>
    using kernel = kernel_function_impl<native_data_type>;
    using handle = gpu_module_base::handle;

    ipc_gpu_typed_module(handle module_handle) :
     gpu_module_base(nullptr)
    {
        LOG_DEBUG("Remote gpu module created: ", ccl_coll_type_to_str(type));
        main_function_args.handle = nullptr; //no handle
        main_function_args_int.handle = nullptr; //no handle
        LOG_DEBUG("No need to import functions");
    }

    //TODO - use std::tuple  for multiple types support
    kernel<float> main_function_args;
    kernel<int> main_function_args_int;

protected:
    ~ipc_gpu_typed_module() = default;
};


//3) virtual gpu module
template<ccl_coll_type type,
         template<typename> class kernel_function_impl>
struct virtual_gpu_typed_module : private gpu_module_base
{
    using real_referenced_module = real_gpu_typed_module<type, kernel_function_impl>;

    template<class native_data_type>
    using kernel = typename real_referenced_module::template kernel<native_data_type>;   //The same as real
    using handle = typename real_referenced_module::handle;

    virtual_gpu_typed_module(std::shared_ptr<real_referenced_module> real_module) :
        gpu_module_base(real_module->get()),
        real_module_ref(real_module)
    {
        LOG_DEBUG("Virtual gpu module created:", ccl_coll_type_to_str(type));
        main_function_args.handle = import_kernel(kernel<float>::name());
        main_function_args_int.handle = import_kernel(kernel<int>::name());
        LOG_DEBUG("Linked functions count: ", functions.size());
    }

    kernel<float> main_function_args;
    kernel<int> main_function_args_int;

    std::shared_ptr<real_referenced_module> real_module_ref;

protected:
    ~virtual_gpu_typed_module()
    {
        module = nullptr;   //real module owner will destroy it
        release();
    }
};



#define DEFINE_SPECIFIC_GPU_MODULE_CLASS(module_type, base_module_type, coll_type, topology, export_function)     \
template<>                                                                                                        \
struct module_type<coll_type, topology> :                                                                         \
       public base_module_type<coll_type, export_function>                                                        \
{                                                                                                                 \
    using base = base_module_type<coll_type, export_function>;                                                    \
    using base::kernel;                                                                                           \
    using base::handle;                                                                                           \
    static constexpr ccl_coll_type get_coll_type() { return coll_type; }                                          \
    static constexpr ccl::device_topology_type get_topology_type() { return topology; }                           \
                                                                                                                  \
    module_type<coll_type, topology>(handle module_handle):                                                       \
        base(module_handle)                                                                                       \
    {                                                                                                             \
    }                                                                                                             \
}


#define DEFINE_VIRTUAL_GPU_MODULE_CLASS(coll_type, topology, export_function)                                       \
template<>                                                                                                          \
struct virtual_gpu_coll_module<coll_type, topology> :                                                               \
       public virtual_gpu_typed_module<coll_type, export_function>                                                  \
{                                                                                                                   \
    using base = virtual_gpu_typed_module<coll_type, export_function>;                                              \
    using base::kernel;                                                                                             \
    using base::handle;                                                                                             \
    using real_referenced_module = typename base::real_referenced_module;                                           \
    static constexpr ccl_coll_type get_coll_type() { return coll_type; }                                            \
    static constexpr ccl::device_topology_type get_topology_type() { return topology; }                             \
                                                                                                                    \
    virtual_gpu_coll_module<coll_type, topology>(std::shared_ptr<real_referenced_module> real_module):              \
        base(real_module)                                                                                           \
    {                                                                                                               \
    }                                                                                                               \
}

}
