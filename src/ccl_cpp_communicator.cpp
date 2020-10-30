#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_aliases.hpp"

#include "oneapi/ccl/ccl_type_traits.hpp"
#include "oneapi/ccl/ccl_types_policy.hpp"

#include "oneapi/ccl/ccl_coll_attr_ids.hpp"
#include "oneapi/ccl/ccl_coll_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_coll_attr.hpp"

#include "oneapi/ccl/ccl_comm_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_attr.hpp"

#include "oneapi/ccl/ccl_comm_split_attr_ids.hpp"
#include "oneapi/ccl/ccl_comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_comm_split_attr.hpp"

#include "oneapi/ccl/ccl_stream_attr_ids.hpp"
#include "oneapi/ccl/ccl_stream_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_stream.hpp"

#include "oneapi/ccl/ccl_device_attr_ids.hpp"
#include "oneapi/ccl/ccl_device_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_device.hpp"

#include "oneapi/ccl/ccl_context_attr_ids.hpp"
#include "oneapi/ccl/ccl_context_attr_ids_traits.hpp"
#include "oneapi/ccl/ccl_context.hpp"

#include "oneapi/ccl/ccl_event.hpp"

#include "oneapi/ccl/ccl_communicator.hpp"
#include "common/comm/l0/comm_context_storage.hpp"

#include "common/global/global.hpp"

//TODO
#include "common/comm/comm.hpp"

#include "common/comm/l0/comm_context.hpp"
#include "communicator_impl.hpp"

namespace ccl {

namespace v1 {

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

CCL_API communicator::~communicator() {}

CCL_API int communicator::rank() const {
    return get_impl()->rank();
}

CCL_API int communicator::size() const {
    return get_impl()->size();
}

/*CCL_API size_t communicator::get_group_unique_id() const
{
    return static_cast<size_t> (get_impl()->get_comm_group_id());
}*/

CCL_API communicator communicator::split(const comm_split_attr& attr) {
    return communicator(get_impl()->split(attr));
}

CCL_API device communicator::get_device() const {
    return device::create_device(get_impl()->get_device());
}

CCL_API context communicator::get_context() const {
    return context::create_context(get_impl()->get_context());
}

} // namespace v1

} // namespace ccl

/****API force instantiations for factory methods******/
API_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(ccl::device, ccl::context)
API_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(ccl::device,
                                                                  ccl::context)
API_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(ccl::device, ccl::context)

API_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(typename ccl::unified_device_type::ccl_native_t, typename ccl::unified_context_type::ccl_native_t)
API_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(typename ccl::unified_device_type::ccl_native_t, typename ccl::unified_context_type::ccl_native_t)
API_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(typename ccl::unified_device_type::ccl_native_t, typename ccl::unified_context_type::ccl_native_t)

API_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    typename ccl::unified_context_type::ccl_native_t)
API_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    typename ccl::unified_context_type::ccl_native_t)
API_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(
    ccl::device_index_type,
    typename ccl::unified_context_type::ccl_native_t)
