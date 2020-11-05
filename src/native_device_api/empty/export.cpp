#include "oneapi/ccl/config.h"
#if !defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)

#include "oneapi/ccl/native_device_api/empty/export.hpp"
#include "oneapi/ccl/type_traits.hpp"
#include "common/log/log.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace ccl {

generic_context_type<cl_backend_type::empty_backend>::ccl_native_t
generic_context_type<cl_backend_type::empty_backend>::get() noexcept {
    return /*const_cast<generic_context_type<cl_backend_type::l0>::ccl_native_t>*/ (
        static_cast<const generic_context_type<cl_backend_type::empty_backend>*>(this)->get());
}

const generic_context_type<cl_backend_type::empty_backend>::ccl_native_t&
generic_context_type<cl_backend_type::empty_backend>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
} // namespace ccl

#endif //#if !defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)
