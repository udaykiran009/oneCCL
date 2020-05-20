#include "common/comm/comm_interface.hpp"
#include "common/comm/compiler_comm_interface_dispatcher_impl.hpp"

#ifdef MULTI_GPU_SUPPORT
COMMUNICATOR_INTERFACE_DISPATCHER_NON_CLASS_EXPLICIT_INSTANTIATION(ccl::device_index_type);

#ifdef CCL_ENABLE_SYCL
    COMMUNICATOR_INTERFACE_DISPATCHER_CLASS_EXPLICIT_INSTANTIATION(cl::sycl::device);
#endif

#endif
