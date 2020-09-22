#pragma once
#include "oneapi/ccl/ccl_types.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

namespace native {
namespace details {

#ifdef CCL_ENABLE_SYCL
size_t get_sycl_device_id(const cl::sycl::device& dev);
std::string usm_to_string(cl::sycl::usm::alloc val);
#endif

enum usm_support_mode { prohibited = 0, direct, shared, need_conversion, last_value };
std::string to_string(usm_support_mode val);

using assoc_retult = std::tuple<usm_support_mode, const void*, std::string>;
enum assoc_result_index { SUPPORT_MODE = 0, POINTER_VALUE, ERROR_CAUSE };

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
assoc_retult check_assoc_device_memory(const void* mem,
                                       const ccl::unified_device_type::ccl_native_t& device,
                                       const ccl::unified_device_context_type::ccl_native_t& ctx);

#endif //defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
std::string to_string(const assoc_retult& res);

#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
template <size_t N>
using multiple_assoc_result = std::array<assoc_retult, N>;

template <class... mem_type>
auto check_multiple_assoc_device_memory(const ccl::unified_device_type::ccl_native_t& device,
                                        const ccl::unified_device_context_type::ccl_native_t& ctx,
                                        const mem_type*... mem)
    -> multiple_assoc_result<sizeof...(mem)> {
    multiple_assoc_result<sizeof...(mem)> ret{ check_assoc_device_memory(mem, device, ctx)... };
    return ret;
}

template <size_t N>
std::string to_string(const multiple_assoc_result<N>& res) {
    std::stringstream ss;
    for (size_t i = 0; i < N; i++) {
        ss << "Arg: " << std::to_string(i) << to_string(res[i]) << std::endl;
    }
    return ss.str();
}
#endif //defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
} // namespace details
} // namespace native
