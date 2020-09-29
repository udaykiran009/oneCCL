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

#include "common/event_internal/ccl_event/ccl_event_attr_ids.hpp"
#include "common/event_internal/ccl_event/ccl_event_attr_ids_traits.hpp"
#include "common/event_internal/ccl_event/ccl_event.hpp"

#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "oneapi/ccl/ccl_event.hpp"

#include "oneapi/ccl/ccl_communicator.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "common/global/global.hpp"

//TODO
#include "common/comm/comm.hpp"

#include "common/comm/l0/comm_context.hpp"
#include "communicator_impl.hpp"

namespace ccl {
CCL_API communicator::communicator(impl_value_t&& impl) : base_t(std::move(impl)) {}

CCL_API communicator::communicator(communicator&& src)
        : base_t(std::move(src)) {}

CCL_API communicator& communicator::operator=(communicator&& src) {
    if (src.get_impl() != this->get_impl()) {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

CCL_API ccl::communicator::~communicator() {}

CCL_API size_t ccl::communicator::rank() const {
    return get_impl()->rank();
}

CCL_API size_t ccl::communicator::size() const {
    return get_impl()->size();
}

/*CCL_API size_t ccl::communicator::get_group_unique_id() const
{
    return static_cast<size_t> (get_impl()->get_comm_group_id());
}*/

CCL_API ccl::communicator ccl::communicator::split(
    const ccl::comm_split_attr& attr) {
    if (!attr.is_valid<ccl::comm_split_attr_id::group>()) {
        throw ccl::exception(std::string(__FUNCTION__) +
                        " - TODO `comm_split_attr`: supports `group` only");
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
    throw ccl::exception(std::string(__FUNCTION__) + " - TODO `comm_split_attr`: unsupported");
    return std::move(*this);
#endif
}

CCL_API ccl::communicator::ccl_device_t ccl::communicator::get_device() {
    return get_impl()->get_device();
}

CCL_API ccl::communicator::ccl_context_t ccl::communicator::get_context() {
    return get_impl()->get_context();
}

} // namespace ccl

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
