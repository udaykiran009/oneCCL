#pragma once

#include "common/comm/l0/modules/base_entry_module.hpp"
#include "common/utils/tuple.hpp"

namespace native {
namespace detail {

template <ccl_coll_type type>
struct kernel_entry_initializer {
    using loader_t =
        std::function<gpu_module_base::kernel_handle(const std::string& function_name)>;

    kernel_entry_initializer(loader_t&& f) : functor(std::move(f)) {}

    template <class typed_kernel>
    void operator()(typed_kernel& kernel) {
        kernel.handle =
            functor(std::string(typed_kernel::name()) + "_" +
                    ccl::native_type_info<typename typed_kernel::processing_type>::name());
    }

private:
    loader_t functor;
};

template <>
struct kernel_entry_initializer <ccl_coll_allreduce> {
    using loader_t =
        std::function<gpu_module_base::kernel_handle(const std::string& function_name)>;

    kernel_entry_initializer(loader_t&& f) : functor(std::move(f)) {}

    template <class typed_kernel>
    void operator()(typed_kernel& kernel) {
        kernel.handle =
            functor(std::string(typed_kernel::name()) + "_" +
                    ccl::native_type_info<typename typed_kernel::processing_type>::name() + "_add");
    }
private:
    loader_t functor;
};

template <>
struct kernel_entry_initializer <ccl_coll_reduce> {
    using loader_t =
        std::function<gpu_module_base::kernel_handle(const std::string& function_name)>;

    kernel_entry_initializer(loader_t&& f) : functor(std::move(f)) {}

    template <class typed_kernel>
    void operator()(typed_kernel& kernel) {
        kernel.handle =
            functor(std::string(typed_kernel::name()) + "_" +
                    ccl::native_type_info<typename typed_kernel::processing_type>::name() + "_add");
    }

private:
    loader_t functor;
};

} // namespace detail
} // namespace native
