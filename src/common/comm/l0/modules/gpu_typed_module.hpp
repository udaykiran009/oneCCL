#pragma once
#include <type_traits>
#include <memory>
#include "common/comm/l0/modules/base_entry_module.hpp"
#include "common/comm/l0/modules/modules_utils.hpp"
#include "common/comm/l0/communicator/base_communicator.hpp"

namespace native {

// The get_kernel_type is a DEFAULT template class of kernels, which DON'T use reduction.
template <
    ccl_coll_type type,
    template <typename>
    class kernel_function_impl, // template function with a varidaic template parametr
    template <typename>
    class kernel_numa_function_impl, // template function of NUMA with a varidaic template parametr
    typename = void>
struct get_kernel_type {
    // Template for ONE kernel, reduction is add by default
    template <class native_data_type, ccl_coll_reduction red_type = ccl_coll_reduction::add>
    using kernel = kernel_function_impl<kernel_params_default<native_data_type>>;

    // Template for SEVERAL kernels for SEVERAL(variadic) data types, which is based on 'using kernel'.
    // Reduction is add by default
    template <class... native_data_types>
    using kernels = std::tuple<kernel<native_data_types>...>;

    // the same approach as above, but for NUMA using
    template <class native_data_type, ccl_coll_reduction red_type = ccl_coll_reduction::add>
    using kernel_numa = kernel_numa_function_impl<kernel_params_default<native_data_type>>;

    template <class... native_data_types>
    using kernels_numa = std::tuple<kernel_numa<native_data_types>...>;

    template <class kernel_params>
    using get_kernel = kernel<typename kernel_params::native_type>;

    template <class kernel_params>
    using get_kernel_numa = kernel_numa<typename kernel_params::native_type>;
};

// The get_kernel_type is a SPECIALIZATION template class of kernels, which USE template reduction type.
template <
    ccl_coll_type type,
    template <typename>
    class kernel_function_impl, // template function with a varidaic template parametr
    template <typename>
    class kernel_numa_function_impl> // template function of NUMA with a varidaic template parametr
// SPECIALIZATION for allreduce, reduce, reduce_scatter
struct get_kernel_type<
    type,
    kernel_function_impl,
    kernel_numa_function_impl,
    typename std::enable_if<type == ccl_coll_allreduce || type == ccl_coll_reduce ||
                            type == ccl_coll_reduce_scatter>::type> {
    // Template for ONE kernel, reduction is given by user
    template <class native_data_type, ccl_coll_reduction red_type>
    using kernel = kernel_function_impl<kernel_params_traits<native_data_type, red_type>>;

    // Variadic template kernel is consists of ONE(template) data type and SEVERAL(variadic) reductions, which is based on 'using kernel'.
    // Reductions are given by user. The template is instantiated into tuple ONLY for reductions.
    template <class native_data_type, ccl_coll_reduction... reductions>
    using kernel_impl = std::tuple<kernel<native_data_type, reductions>...>;

    // Variadic template for SEVERAL kernels for SEVERAL(variadic) data types and ONE reduction.
    // std::tuple_cat makes a concatenation of all(reductions, data types) tuples in one.
    // This reception provided us the ONE tuple of all combinations of data types, reductions:
    // Exmaple(1): std::tuple T1('allreduce_execution_int8_add','allreduce_execution_int8_max' ..//and so on);
    // if we don't apply this reception, example(2):
    // std::tuple T1('allreduce_execution_int8_add','allreduce_execution_int16_add' ..//and so on);
    // std::tuple T2('allreduce_execution_int8_max','allreduce_execution_int16_max' ..//and so on);
    // We don't want to iterate over two tuples, having one external and inner for_each().
    // Applying example (1), it is enough to have one for_each, iterating over one universal tuple.
    template <class... native_data_types>
    using kernels =
        decltype(std::tuple_cat(std::declval<kernel_impl<native_data_types, REDUCE_TYPES>&&>()...));

    template <class native_data_type, ccl_coll_reduction red_type>
    using kernel_numa = kernel_numa_function_impl<kernel_params_traits<native_data_type, red_type>>;

    template <class native_data_type, ccl_coll_reduction... reductions>
    using kernel_numa_impl = std::tuple<kernel_numa<native_data_type, reductions>...>;

    template <class... native_data_types>
    using kernels_numa = decltype(
        std::tuple_cat(std::declval<kernel_numa_impl<native_data_types, REDUCE_TYPES>&&>()...));

    template <class kernel_params>
    using get_kernel = kernel<typename kernel_params::native_type, kernel_params::red_type>;

    template <class kernel_params>
    using get_kernel_numa =
        kernel_numa<typename kernel_params::native_type, kernel_params::red_type>;
};

// default or specialization get_kernel_type is applied to real_gpu_typed_module, ipc_gpu_typed_module, virtual_gpu_typed_module.
template <ccl_coll_type type,
          template <typename>
          class kernel_function_impl,
          template <typename>
          class kernel_numa_function_impl>
struct real_gpu_typed_module : private gpu_module_base {
    // Here applying get_kernel_type, where chosing between default or specialization get_kernel_type is based on 'type'
    template <class native_data_type, ccl_coll_reduction red_type = ccl_coll_reduction::add>
    using kernel = typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
        template kernel<native_data_type, red_type>;

    template <class... native_data_types>
    using kernels =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels<native_data_types...>;

    // the same as above
    template <class native_data_type, ccl_coll_reduction reductions = ccl_coll_reduction::add>
    using kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernel_numa<native_data_type, reductions>;

    template <class... native_data_types>
    using kernels_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels_numa<native_data_types...>;

    template <class kernel_params>
    using get_kernel =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel<kernel_params>;

    template <class kernel_params>
    using get_kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel_numa<kernel_params>;

    using handle = gpu_module_base::handle;

    real_gpu_typed_module(handle module_handle) : gpu_module_base(module_handle) {
        LOG_DEBUG("Real gpu module created: ",
                  ccl_coll_type_to_str(type),
                  ", modules handle: ",
                  (void*)module);
        ccl_tuple_for_each(kernel_main_functions,
                           detail::kernel_entry_initializer<type>(
                               [this](const std::string& name) -> gpu_module_base::kernel_handle {
                                   return this->import_kernel(name);
                               }));

        ccl_tuple_for_each(kernel_numa_functions,
                           detail::kernel_entry_initializer<type>(
                               [this](const std::string& name) -> gpu_module_base::kernel_handle {
                                   return this->import_kernel(name);
                               }));

        LOG_DEBUG("Imported functions count: ", functions.size());
    }

    handle get() const {
        return module;
    }

    template <class kernel_params>
    get_kernel<kernel_params>& get_main_function() {
        return const_cast<get_kernel<kernel_params>&>(
            static_cast<const real_gpu_typed_module*>(this)->get_main_function<kernel_params>());
    }

    template <class kernel_params>
    const get_kernel<kernel_params>& get_main_function() const {
        return ccl_tuple_get<get_kernel<kernel_params>>(kernel_main_functions);
    }

    template <class kernel_params>
    get_kernel_numa<kernel_params>& get_numa_main_function() {
        return const_cast<get_kernel_numa<kernel_params>&>(
            static_cast<const real_gpu_typed_module*>(this)
                ->get_numa_main_function<kernel_params>());
    }

    template <class kernel_params>
    const get_kernel_numa<kernel_params>& get_numa_main_function() const {
        return ccl_tuple_get<get_kernel_numa<kernel_params>>(kernel_numa_functions);
    }

    ~real_gpu_typed_module() {
        LOG_DEBUG("Real gpu module destroyed: ",
                  ccl_coll_type_to_str(type),
                  ", modules handle: ",
                  (void*)module);
    }

protected:
    kernels<SUPPORTED_KERNEL_NATIVE_DATA_TYPES> kernel_main_functions;
    kernels_numa<SUPPORTED_KERNEL_NATIVE_DATA_TYPES> kernel_numa_functions;
};

//2) virtual ipc_gpu_typed_module
template <ccl_coll_type type,
          template <typename>
          class kernel_function_impl,
          template <typename>
          class kernel_numa_function_impl>
struct ipc_gpu_typed_module : private gpu_module_base {
    template <class native_data_type, ccl_coll_reduction red_type = ccl_coll_reduction::add>
    using kernel = typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
        template kernel<native_data_type, red_type>;

    template <class... native_data_types>
    using kernels =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels<native_data_types...>;

    template <class native_data_type, ccl_coll_reduction reductions = ccl_coll_reduction::add>
    using kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernel_numa<native_data_type, reductions>;

    template <class... native_data_types>
    using kernels_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels_numa<native_data_types...>;

    template <class kernel_params>
    using get_kernel =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel<kernel_params>;

    template <class kernel_params>
    using get_kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel_numa<kernel_params>;

    using handle = gpu_module_base::handle;

    ipc_gpu_typed_module(handle module_handle) : gpu_module_base(nullptr) {
        LOG_DEBUG("Remote gpu module created: ", ccl_coll_type_to_str(type));
        ccl_tuple_for_each(kernel_main_functions,
                           detail::kernel_entry_initializer<type>(
                               [](const std::string& name) -> gpu_module_base::kernel_handle {
                                   return nullptr;
                               }));
        LOG_DEBUG("No need to import functions");
    }

    template <class kernel_params>
    get_kernel<kernel_params>& get_main_function() {
        return const_cast<get_kernel<kernel_params>&>(
            static_cast<const ipc_gpu_typed_module*>(this)->get_main_function<kernel_params>());
    }

    template <class kernel_params>
    const get_kernel<kernel_params>& get_main_function() const {
        return ccl_tuple_get<get_kernel<kernel_params>>(kernel_main_functions);
    }

    ~ipc_gpu_typed_module() = default;

protected:
    kernels<SUPPORTED_KERNEL_NATIVE_DATA_TYPES> kernel_main_functions;
};

//3) virtual gpu module
template <ccl_coll_type type,
          template <typename>
          class kernel_function_impl,
          template <typename>
          class kernel_numa_function_impl>
struct virtual_gpu_typed_module : private gpu_module_base {
    // TODO: use real_referenced_module to reduce given params
    using real_referenced_module =
        real_gpu_typed_module<type, kernel_function_impl, kernel_numa_function_impl>;

    template <class native_data_type, ccl_coll_reduction red_type = ccl_coll_reduction::add>
    using kernel = typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
        template kernel<native_data_type, red_type>;

    template <class... native_data_types>
    using kernels =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels<native_data_types...>;

    template <class native_data_type, ccl_coll_reduction reductions = ccl_coll_reduction::add>
    using kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernel_numa<native_data_type, reductions>;

    template <class... native_data_types>
    using kernels_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template kernels_numa<native_data_types...>;

    template <class kernel_params>
    using get_kernel =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel<kernel_params>;

    template <class kernel_params>
    using get_kernel_numa =
        typename get_kernel_type<type, kernel_function_impl, kernel_numa_function_impl>::
            template get_kernel_numa<kernel_params>;
    using handle = typename real_referenced_module::handle;

    virtual_gpu_typed_module(std::shared_ptr<real_referenced_module> real_module)
            : gpu_module_base(real_module->get()),
              real_module_ref(real_module) {
        LOG_DEBUG("Virtual gpu module created:", ccl_coll_type_to_str(type));
        ccl_tuple_for_each(kernel_main_functions,
                           detail::kernel_entry_initializer<type>(
                               [this](const std::string& name) -> gpu_module_base::kernel_handle {
                                   return this->import_kernel(name);
                               }));
        ccl_tuple_for_each(kernel_numa_functions,
                           detail::kernel_entry_initializer<type>(
                               [this](const std::string& name) -> gpu_module_base::kernel_handle {
                                   return this->import_kernel(name);
                               }));

        LOG_DEBUG("Linked functions count: ", functions.size());
    }

    template <class kernel_params>
    get_kernel<kernel_params>& get_main_function() {
        return const_cast<get_kernel<kernel_params>&>(
            static_cast<const virtual_gpu_typed_module*>(this)->get_main_function<kernel_params>());
    }

    template <class kernel_params>
    const get_kernel<kernel_params>& get_main_function() const {
        return ccl_tuple_get<get_kernel<kernel_params>>(kernel_main_functions);
    }

    template <class kernel_params>
    get_kernel_numa<kernel_params>& get_numa_main_function() {
        return const_cast<get_kernel_numa<kernel_params>&>(
            static_cast<const virtual_gpu_typed_module*>(this)
                ->get_numa_main_function<kernel_params>());
    }

    template <class kernel_params>
    const get_kernel_numa<kernel_params>& get_numa_main_function() const {
        return ccl_tuple_get<get_kernel_numa<kernel_params>>(kernel_numa_functions);
    }

    std::shared_ptr<real_referenced_module> real_module_ref;

    ~virtual_gpu_typed_module() {
        LOG_DEBUG("Virtual gpu module destroyed: ",
                  ccl_coll_type_to_str(type),
                  ", modules handle: ",
                  (void*)module);
        module = nullptr; //real module owner will destroy it
        release();
    }

protected:
    kernels<SUPPORTED_KERNEL_NATIVE_DATA_TYPES> kernel_main_functions;
    kernels_numa<SUPPORTED_KERNEL_NATIVE_DATA_TYPES> kernel_numa_functions;
};

#define DEFINE_SPECIFIC_GPU_MODULE_CLASS( \
    module_type, base_module_type, coll_type, mode, export_function, export_numa_function) \
    template <ccl::group_split_type topology> \
    struct module_type<coll_type, topology, mode> \
            : public base_module_type<coll_type, export_function, export_numa_function> { \
        using base = base_module_type<coll_type, export_function, export_numa_function>; \
        using base::kernel; \
        using base::kernel_numa; \
        using base::handle; \
        static constexpr ccl_coll_type get_coll_type() { \
            return coll_type; \
        } \
        static constexpr ccl::group_split_type get_topology_type() { \
            return topology; \
        } \
        static constexpr ccl::device_topology_type get_topology_class() { \
            return mode; \
        } \
\
        module_type<coll_type, topology, mode>(handle module_handle) : base(module_handle) {} \
    }

/*
#define DEFINE_SPECIFIC_GPU_MODULE_CLASS(module_type, base_module_type, export_function, export_numa_function)     \
template<ccl_coll_type type, ccl::group_split_type topology>                                   \
using module_type = base_module_type<type, topology, export_function, export_numa_function>;
*/

#define DEFINE_VIRTUAL_GPU_MODULE_CLASS(coll_type, mode, export_function, export_numa_function) \
    template <ccl::group_split_type topology> \
    struct virtual_device_coll_module<coll_type, topology, mode> \
            : public virtual_gpu_typed_module<coll_type, export_function, export_numa_function> { \
        using base = virtual_gpu_typed_module<coll_type, export_function, export_numa_function>; \
        using base::kernel; \
        using base::kernel_numa; \
        using base::handle; \
        using real_referenced_module = typename base::real_referenced_module; \
        static constexpr ccl_coll_type get_coll_type() { \
            return coll_type; \
        } \
        static constexpr ccl::group_split_type get_topology_type() { \
            return topology; \
        } \
        static constexpr ccl::device_topology_type get_topology_class() { \
            return mode; \
        } \
\
        virtual_device_coll_module<coll_type, topology, mode>( \
            std::shared_ptr<real_referenced_module> real_module) \
                : base(real_module) {} \
    }

} // namespace native
