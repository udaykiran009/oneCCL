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
        return ccl::group_split_type::undetermined;
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
                                          size_t thread_idx,
                                          size_t process_idx,
                                          const ccl::comm_split_attr& attr);

    ccl::group_split_type get_topology_type() const override;
    ccl::device_topology_type get_topology_class() const override;

    bool is_ready() const override;

    // communicator interfaces implementation
    DEVICE_COMM_INTERFACE_COLL_DEFINITION__VOID;
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(char);
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(int);
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(int64_t);
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(uint64_t);
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(float);
    DEVICE_COMM_INTERFACE_COLL_DEFINITION(double);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<char COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<int COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<int64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<uint64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DEFINITION(cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION__VOID;
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, ccl::bf16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, ccl::bf16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, ccl::bf16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, ccl::bf16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int COMMA 1>,
                                                  cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int COMMA 1>,
                                                  cl::sycl::buffer<ccl::bf16 COMMA 1>);

    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int64_t COMMA 1>,
                                                  cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int64_t COMMA 1>,
                                                  cl::sycl::buffer<ccl::bf16 COMMA 1>);
#endif //CCL_ENABLE_SYCL

    // troubleshooting
    std::string to_string() const;

    impl_t* get_impl() {
        return static_cast<impl_t*>(this);
    }
};
