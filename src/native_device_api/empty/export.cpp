#include "oneapi/ccl/ccl_config.h"
#if !defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)

#include "oneapi/ccl/native_device_api/empty/export.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/log/log.hpp"
#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace ccl {

generic_device_context_type<cl_backend_type::empty_backend>::generic_device_context_type(...) {}

generic_device_context_type<cl_backend_type::empty_backend>::ccl_native_t
generic_device_context_type<cl_backend_type::empty_backend>::get() noexcept {
    return /*const_cast<generic_device_context_type<cl_backend_type::l0>::ccl_native_t>*/ (
        static_cast<const generic_device_context_type<cl_backend_type::empty_backend>*>(this)->get());
}

const generic_device_context_type<cl_backend_type::empty_backend>::ccl_native_t&
generic_device_context_type<cl_backend_type::empty_backend>::get() const noexcept {
    //TODO
    return context; //native::get_platform();
}
}

#endif //#if !defined(MULTI_GPU_SUPPORT) and !defined(CCL_ENABLE_SYCL)
