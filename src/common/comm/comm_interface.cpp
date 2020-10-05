#include "common/comm/comm_interface.hpp"
#include "common/comm/compiler_comm_interface_dispatcher_impl.hpp"


COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(typename ccl::unified_device_type::ccl_native_t, typename ccl::unified_device_context_type::ccl_native_t);
COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(ccl::device_index_type, typename ccl::unified_device_context_type::ccl_native_t);
