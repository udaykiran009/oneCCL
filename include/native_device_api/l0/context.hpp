#pragma once
#include <mutex> //TODO use shared

#include "native_device_api/l0/primitives.hpp"
#include "native_device_api/l0/utils.hpp"

namespace native {
struct ccl_device_platform;
struct ccl_device_driver;
struct ccl_subdevice;
struct ccl_device;

// TODO not thread-safe!!!
struct ccl_context : public /*cl_base<ze_device_handle_t, ccl_device_driver>,*/
                    std::enable_shared_from_this<ccl_context>
{
    std::shared_ptr<ccl_context> get_ptr() {
        return this->shared_from_this();
    }
};
}
