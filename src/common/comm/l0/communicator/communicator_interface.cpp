#include "common/comm/l0/communicator/communicator_interface.hpp"
#include "common/comm/l0/communicator/compiler_communicator_interface_dispatcher_impl.hpp"

COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(ccl::device_index_type);

#ifdef CCL_ENABLE_SYCL
    COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(cl::sycl::device);
#endif
