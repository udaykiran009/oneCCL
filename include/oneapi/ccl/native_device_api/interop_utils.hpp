#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/type_traits.hpp"
#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

namespace native {
namespace detail {

#ifdef CCL_ENABLE_SYCL
size_t get_sycl_device_id(const cl::sycl::device& dev);
size_t get_sycl_subdevice_id(const cl::sycl::device& dev);
#endif

} // namespace detail
} // namespace native
