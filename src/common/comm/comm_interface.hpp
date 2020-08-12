#pragma once
#include "common/comm/compiler_comm_interface_dispatcher.hpp"
#include "types_generator_defines.hpp"

namespace native {
struct ccl_device;
}

namespace ccl {
struct gpu_comm_attr;
struct communicator_interface : public communicator_interface_dispatcher {
    virtual ~communicator_interface() = default;

    virtual size_t rank() const = 0;
    virtual size_t size() const = 0;

    virtual bool is_host() const noexcept = 0;
    virtual bool is_cpu() const noexcept = 0;
    virtual bool is_gpu() const noexcept = 0;
    virtual bool is_accelerator() const noexcept = 0;

    virtual bool is_ready() const = 0;

    // collectives operation declarations
    virtual ccl::device_communicator::coll_request_t barrier(
                         const barrier_attr_t& attr,
                         ccl::stream::impl_value_t& op_stream,
                         const vector_class<event>& deps = {}) = 0;

    DEVICE_COMM_INTERFACE_COLL_DECLARATION__VOID;
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(char);
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(int);
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(int64_t);
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(uint64_t);
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(float);
    DEVICE_COMM_INTERFACE_COLL_DECLARATION(double);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<char COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<int COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<int64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<uint64_t COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_COLL_CLASS_DECLARATION(cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION__VOID
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, char);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, int);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, float);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, double);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(char, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, char);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, int);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, float);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, double);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(int64_t, uint64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, char);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, int);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, ccl::bfp16);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, float);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, double);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, int64_t);
    DEVICE_COMM_INTERFACE_SPARSE_DECLARATION(uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DECLARATION(cl::sycl::buffer<int COMMA 1>,
                                            cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DECLARATION(cl::sycl::buffer<int COMMA 1>,
                                            cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DECLARATION(cl::sycl::buffer<int64_t COMMA 1>,
                                            cl::sycl::buffer<float COMMA 1>);
    DEVICE_COMM_INTERFACE_SPARSE_CLASS_DECLARATION(cl::sycl::buffer<int64_t COMMA 1>,
                                            cl::sycl::buffer<ccl::bfp16 COMMA 1>);
#endif //CCL_ENABLE_SYCL
};
} // namespace ccl
