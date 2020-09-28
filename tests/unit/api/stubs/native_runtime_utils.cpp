#include "oneapi/ccl/native_device_api/interop_utils.hpp"
namespace native {
namespace details {
#ifdef CCL_ENABLE_SYCL
size_t get_sycl_device_id(const cl::sycl::device &dev) {
    return 0;
}
#endif
} // namespace details
} // namespace native
