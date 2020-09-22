#pragma once

#include "common/comm/l0/communicator/base_communicator.hpp"
#include "common/comm/l0/device_community_holder.hpp"

/*
namespace native
{
    template<ccl::device_topology_type class_id>
    struct device_community;

    template<ccl::device_topology_type class_id>
    device_community_container;
}
*/
template <class comm_impl,
          ccl::device_group_split_type group_id,
          ccl::device_topology_type class_id,
          class communicator_traits>
class typed_base_communicator : public base_communicator {
public:
    using base_t = base_communicator;
    using impl_t = comm_impl;
    using self_t = typed_base_communicator<comm_impl, group_id, class_id, communicator_traits>;
    using traits = communicator_traits;

    // Topologies
    static constexpr ccl::device_group_split_type topology_type() {
        return group_id;
    }

    static constexpr ccl::device_topology_type topology_class() {
        return class_id;
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

    typed_base_communicator(ccl::unified_device_type&& device,
                            size_t thread_idx,
                            size_t process_idx,
                            const ccl::device_comm_split_attr& attr);

    ccl::device_group_split_type get_topology_type() const override;
    ccl::device_topology_type get_topology_class() const override;

    void initialize_comm_addr(const ccl::device_index_type& device_id,
                              native::device_community_container<class_id>& community);

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
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(char, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(int64_t, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DEFINITION(uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int COMMA 1>,
                                                  cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int COMMA 1>,
                                                  cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int64_t COMMA 1>,
                                                  cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DEFINITION(cl::sycl::buffer<int64_t COMMA 1>,
                                                  cl::sycl::buffer<ccl::bfp16 COMMA 1>);
#endif //CCL_ENABLE_SYCL

    // Device community interface
    /*    template<class device_t>
    size_t get_device_count() const;

    template<class device_t>
    native::indexed_device_container<device_t>& get_devices();
*/
    // troubleshooting
    std::string to_string() const;

    native::device_community_container<class_id> device_community_impl;

    impl_t* get_impl() {
        return static_cast<impl_t*>(this);
    }
};
