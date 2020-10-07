#pragma once
#include <mutex> //TODO use shared

#include "oneapi/ccl/native_device_api/l0/base.hpp"
#include "oneapi/ccl/native_device_api/l0/primitives.hpp"
#include "oneapi/ccl/native_device_api/l0/utils.hpp"

namespace native {
struct ccl_device_platform;
struct ccl_device_driver;
struct ccl_subdevice;
struct ccl_device;

// TODO not thread-safe!!!
struct ccl_context : public cl_base<ze_context_handle_t, ccl_device_platform, ccl_context>,
                     std::enable_shared_from_this<ccl_context> {
    using base = cl_base<ze_context_handle_t, ccl_device_platform, ccl_context>;
    using handle_t = base::handle_t;
    using base::owner_t;
    using base::owner_ptr_t;
    using base::context_t;
    using base::context_ptr_t;

    ccl_context(handle_t h, owner_ptr_t&& platform);

    std::shared_ptr<ccl_context> get_ptr() {
        return this->shared_from_this();
    }

};

struct ccl_context_holder
{
    std::map<ccl_device_driver*, std::vector<std::shared_ptr<ccl_context>>> map_context;

    ze_context_handle_t get() {
        return nullptr;
    }
};

} // namespace native
