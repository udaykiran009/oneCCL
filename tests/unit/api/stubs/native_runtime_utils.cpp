#include "native_device_api/l0/utils.hpp"
namespace native
{
namespace details
{
#ifdef CCL_ENABLE_SYCL
size_t get_sycl_device_id(const cl::sycl::device &dev)
{
    return 0;
}
#endif
}
}
