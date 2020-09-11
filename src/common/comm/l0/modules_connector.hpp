#pragma once
#include "common/comm/l0/base_connector.hpp"
#include "common/utils/tuple.hpp"

template <class managed_kernel, class entry, class... binded_kernels>
struct kernel_connector : public base_connector_interface<managed_kernel> {
    using base = base_connector_interface<managed_kernel>;
    using binding_type = std::tuple<std::reference_wrapper<binded_kernels>...>;
    kernel_connector(entry& e, binded_kernels&... args)
            : base(),
              executor(e),
              deferred_kernels(args...) {}

    bool operator()(managed_kernel& kernel_to_connect) override {
        return connect_impl(
            kernel_to_connect,
            typename sequence_generator<std::tuple_size<binding_type>::value>::type());
    }

private:
    template <int... connected_arguments_indices>
    bool connect_impl(managed_kernel& kernel_to_connect,
                      numeric_sequence<connected_arguments_indices...>) {
        return executor.execute(kernel_to_connect,
                                std::get<connected_arguments_indices>(deferred_kernels).get()...);
    }

    entry& executor;
    binding_type deferred_kernels;
};
