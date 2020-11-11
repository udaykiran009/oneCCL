#pragma once

#include "common/comm/l0/communicator/base_communicator.hpp"

template <class comm_impl, class communicator_traits>
class typed_single_device_base_communicator : public base_communicator {
public:
    using base_t = base_communicator;
    using impl_t = comm_impl;
    using self_t = typed_single_device_base_communicator<comm_impl, communicator_traits>;
    using traits = communicator_traits;

    // Topologies
    static constexpr ccl::group_split_type topology_type() {
        return ccl::group_split_type::single;
    }

    static constexpr ccl::device_topology_type topology_class() {
        return ccl::device_topology_type::undetermined;
    }

    // traits
    bool is_host() const noexcept override {
        return traits::is_host();
    }

    bool is_cpu() const noexcept override {
        return traits::is_cpu();
    }

    bool is_gpu() const noexcept override {
        return traits::is_gpu();
    }

    bool is_accelerator() const noexcept override {
        return traits::is_accelerator();
    }

    typed_single_device_base_communicator(ccl::unified_device_type&& device,
                                          ccl::unified_context_type&& context,
                                          size_t thread_idx,
                                          size_t process_idx,
                                          const ccl::comm_split_attr& attr);

    ccl::group_split_type get_topology_type() const override;
    ccl::device_topology_type get_topology_class() const override;

    bool is_ready() const override;

    COMM_INTERFACE_COLL_METHODS(DEFINITION);
#ifdef CCL_ENABLE_SYCL
    SYCL_COMM_INTERFACE_COLL_METHODS(DEFINITION);
#endif /* CCL_ENABLE_SYCL */

    // troubleshooting
    std::string to_string() const;

    impl_t* get_impl() {
        return static_cast<impl_t*>(this);
    }
};
