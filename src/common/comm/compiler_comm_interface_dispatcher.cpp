#include "common/comm/compiler_comm_interface_dispatcher.hpp"

#include "common/comm/comm_interface.hpp"

#include "oneapi/ccl/types_policy.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "oneapi/ccl/comm_split_attr_ids_traits.hpp"
#include "oneapi/ccl/comm_split_attr.hpp"

#include "common/comm/comm_common_attr.hpp"
#include "common/comm/comm_impl.hpp"
#include "common/comm/comm_split_common_attr.hpp"
#include "common/global/global.hpp"

#include "comm_attr_impl.hpp"
#include "comm_split_attr_impl.hpp"

#ifdef CCL_ENABLE_STUB_BACKEND
#include "common/comm/stub_comm.hpp"
#endif

#include "kvs_impl.hpp"

namespace ccl {

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl() {
    CCL_THROW_IF_NOT(ccl::global_data::env().backend == backend_mode::native,
                     "host communicator is only supported for native backend");

    return communicator_interface_ptr(new ccl_comm());
}

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const size_t size,
    shared_ptr_class<kvs_interface> kvs) {
    CCL_THROW_IF_NOT(ccl::global_data::env().backend == backend_mode::native,
                     "host communicator is only supported for native backend");

    return communicator_interface_ptr(ccl_comm::create(size, kvs));
}

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const size_t size,
    const int rank,
    shared_ptr_class<kvs_interface> kvs) {
    CCL_THROW_IF_NOT(ccl::global_data::env().backend == backend_mode::native,
                     "host communicator is only supported for native backend");

    return communicator_interface_ptr(ccl_comm::create(size, rank, kvs));
}

communicator_interface_ptr communicator_interface_dispatcher::create_communicator_impl(
    const size_t size,
    const int rank,
    device_t device,
    context_t context,
    shared_ptr_class<kvs_interface> kvs) {
#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    if (ccl::global_data::env().backend == backend_mode::native) {
        CCL_THROW_IF_NOT(ccl::global_data::get().ze_data, "ze_data was not initialized");
    }
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_STUB_BACKEND
    if (ccl::global_data::env().backend == backend_mode::stub) {
        return communicator_interface_ptr(ccl::stub_comm::create(device, context, size, rank, kvs));
    }
#endif // CCL_ENABLE_STUB_BACKEND

    return communicator_interface_ptr(ccl_comm::create(device, context, size, rank, kvs));
}

} // namespace ccl
