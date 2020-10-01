#include "oneapi/ccl/native_device_api/l0/utils.hpp"

#if defined(MULTI_GPU_SUPPORT)
#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"

#if defined(CCL_ENABLE_SYCL)
//#ifdef CCL_ENABLE_SYCL
#include <CL/sycl/backend/Intel_level0.hpp>
//static cl::sycl::vector_class<cl::sycl::device> gpu_sycl_devices;
#endif

namespace native {
namespace details {

adjacency_matrix::adjacency_matrix(std::initializer_list<typename base::value_type> init)
        : base(init) {}

cross_device_rating binary_p2p_rating_calculator(const native::ccl_device& lhs,
                                                 const native::ccl_device& rhs,
                                                 size_t weight) {
    return property_p2p_rating_calculator(lhs, rhs, 1);
}
} // namespace details
} // namespace native
#endif //#if defined(MULTI_GPU_SUPPORT)
