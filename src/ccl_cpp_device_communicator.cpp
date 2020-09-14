#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"

#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

#include "oneapi/ccl/ccl_event_attr_ids.hpp"
#include "oneapi/ccl/ccl_event_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_event.hpp"

#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "oneapi/ccl/ccl_request.hpp"

#include "oneapi/ccl/ccl_device_communicator.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "common/global/global.hpp"

//TODO
#include "common/comm/comm.hpp"

#include "common/comm/l0/comm_context.hpp"
#include "device_communicator_impl.hpp"

#ifndef COMMA
#define COMMA ,
#endif

namespace ccl
{
CCL_API device_communicator::device_communicator(impl_value_t&& impl) : base_t(std::move(impl))
{}

CCL_API device_communicator::device_communicator(device_communicator&& src)
        : base_t(std::move(src)) {}

CCL_API device_communicator& device_communicator::operator=(device_communicator&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

CCL_API ccl::device_communicator::~device_communicator() {}

CCL_API size_t ccl::device_communicator::rank() const {
    return get_impl()->rank();
}

CCL_API size_t ccl::device_communicator::size() const {
    return get_impl()->size();
}

/*CCL_API size_t ccl::device_communicator::get_group_unique_id() const
{
    return static_cast<size_t> (get_impl()->get_comm_group_id());
}*/

CCL_API ccl::device_communicator ccl::device_communicator::split(
    const ccl::device_comm_split_attr& attr) {
    if (!attr.is_valid<ccl::comm_split_attr_id::group>()) {
        throw ccl_error(std::string(__FUNCTION__) +
                        " - TODO `device_comm_split_attr`: supports `group` only");
    }
    //TODO
#ifdef MULTI_GPU_SUPPORT
    auto id = get_impl()->get_comm_group_id();
    ccl::group_context::comm_group_t my_group =
        ccl::group_context::instance().get_existing_group_by_id(id);
#ifdef CCL_ENABLE_SYCL
    return my_group->create_communicator<cl::sycl::device>(get_device(), attr);
#else
#ifdef MULTI_GPU_SUPPORT
    return my_group->create_communicator(get_impl()->get_device_path(), attr);
#endif
#endif
#else
    throw ccl_error(std::string(__FUNCTION__) + " - TODO `device_comm_split_attr`: unsupported");
    return std::move(*this);
#endif
}

CCL_API ccl::device_communicator::ccl_device_t ccl::device_communicator::get_device() {
    return get_impl()->get_device();
}

CCL_API ccl::device_communicator::ccl_context_t ccl::device_communicator::get_context() {
    return get_impl()->get_context();
}

}


/****API force instantiations for factory methods******/
#ifdef CCL_ENABLE_SYCL
API_DEVICE_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(cl::sycl::device, cl::sycl::context)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(cl::sycl::device,
                                                                  cl::sycl::context)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(cl::sycl::device, cl::sycl::context)

API_DEVICE_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(ccl::device_index_type, cl::sycl::context)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(ccl::device_index_type,
                                                                  cl::sycl::context)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(ccl::device_index_type,
                                                               cl::sycl::context)
#else
#ifdef MULTI_GPU_SUPPORT
API_DEVICE_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    ccl::unified_device_context_type::ccl_native_t)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    ccl::unified_device_context_type::ccl_native_t)
API_DEVICE_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    ccl::unified_device_context_type::ccl_native_t)
#endif
#endif

// API force instantiations for Operations
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(char);
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(int);
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(int64_t);
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t);
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(float);
API_DEVICE_COMM_OP_PTR_EXPLICIT_INSTANTIATION(double);

#ifdef CCL_ENABLE_SYCL
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<char COMMA 1>);
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int COMMA 1>);
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>);
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<uint64_t COMMA 1>);
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<float COMMA 1>);
API_DEVICE_COMM_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<double COMMA 1>);
#endif //CCL_ENABLE_SYCL

// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, char);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, int);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, ccl::bfp16);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, float);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, double);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, int64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(char, uint64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, char);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, int);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, ccl::bfp16);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, float);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, double);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, int64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int, uint64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, char);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, int);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, ccl::bfp16);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, float);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, double);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, int64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(int64_t, uint64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, char);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, int);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, ccl::bfp16);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, float);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, double);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, int64_t);
// API_DEVICE_COMM_SPARSE_OP_PTR_EXPLICIT_INSTANTIATION(uint64_t, uint64_t);

#ifdef CCL_ENABLE_SYCL
/*
    API_DEVICE_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int COMMA 1>,
                                                      cl::sycl::buffer<float COMMA 1>);
    API_DEVICE_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int COMMA 1>,
                                                      cl::sycl::buffer<ccl::bfp16 COMMA 1>);

    API_DEVICE_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>,
                                                      cl::sycl::buffer<float COMMA 1>);
    API_DEVICE_COMM_SPARSE_OP_REF_EXPLICIT_INSTANTIATION(cl::sycl::buffer<int64_t COMMA 1>,
                                                      cl::sycl::buffer<ccl::bfp16 COMMA 1>);
    */
#endif //CCL_ENABLE_SYCL
#undef COMMA
