#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/aliases.hpp"

#include "oneapi/ccl/type_traits.hpp"
#include "oneapi/ccl/types_policy.hpp"

#include "oneapi/ccl/coll_attr_ids.hpp"
#include "oneapi/ccl/coll_attr_ids_traits.hpp"
#include "oneapi/ccl/coll_attr.hpp"

#include "oneapi/ccl/comm_attr_ids.hpp"
#include "oneapi/ccl/comm_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_attr.hpp"

#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

#include "oneapi/ccl/stream_attr_ids.hpp"
#include "oneapi/ccl/stream_attr_ids_traits.hpp"
#include "oneapi/ccl/stream.hpp"

#include "oneapi/ccl/device_attr_ids.hpp"
#include "oneapi/ccl/device_attr_ids_traits.hpp"
#include "oneapi/ccl/device.hpp"

#include "oneapi/ccl/context_attr_ids.hpp"
#include "oneapi/ccl/context_attr_ids_traits.hpp"
#include "oneapi/ccl/context.hpp"

#include "oneapi/ccl/event.hpp"

#include "oneapi/ccl/communicator.hpp"

#include "common/global/global.hpp"

//TODO
#include "comm/comm.hpp"

#include "communicator_impl.hpp"

namespace ccl {

namespace v1 {

CCL_API communicator::communicator(impl_value_t&& impl) : base_t(std::move(impl)) {}

CCL_API communicator::communicator(communicator&& src) : base_t(std::move(src)) {}

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

CCL_API communicator communicator::split(const comm_split_attr& attr) {
    return communicator(get_impl()->split(attr));
}

} // namespace v1

} // namespace ccl

/****API force instantiations for factory methods******/
API_COMM_CREATE_WO_RANK_EXPLICIT_INSTANTIATION(ccl::device, ccl::context)
API_COMM_CREATE_WITH_RANK_IN_VECTOR_EXPLICIT_INSTANTIATION(ccl::device, ccl::context)
API_COMM_CREATE_WITH_RANK_IN_MAP_EXPLICIT_INSTANTIATION(ccl::device, ccl::context)
