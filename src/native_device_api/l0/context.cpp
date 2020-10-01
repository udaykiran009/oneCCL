#include "oneapi/ccl/native_device_api/l0/context.hpp"
#include "oneapi/ccl/native_device_api/l0/base_impl.hpp"
#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "oneapi/ccl/native_device_api/l0/primitives_impl.hpp"
#include "oneapi/ccl/native_device_api/l0/driver.hpp"
#include "oneapi/ccl/native_device_api/l0/platform.hpp"

#include "native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

namespace native {

  ccl_context::ccl_context(handle_t h, owner_ptr_t&& platform): 
            base(h, std::move(platform), std::weak_ptr<ccl_context> { }) {  }
}// namespace native
