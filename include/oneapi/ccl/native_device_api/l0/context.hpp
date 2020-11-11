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

    std::string to_string() const;
};

class context_array_t {
public:
    using value_type = std::vector<std::shared_ptr<ccl_context>>;
    using context_array_accessor = detail::unique_accessor<std::mutex, value_type>;
    using const_context_array_accessor = detail::unique_accessor<std::mutex, const value_type>;

    context_array_accessor access();
    const_context_array_accessor access() const;

private:
    mutable std::mutex m;
    value_type contexts;
};

struct ccl_context_holder {
    ze_context_handle_t get();
    std::shared_ptr<ccl_context> emplace(ccl_device_driver* driver,
                                         std::shared_ptr<ccl_context>&& ctx);
    context_array_t& get_context_storage(ccl_device_driver* driver);
    const context_array_t& get_context_storage(const ccl_device_driver* driver) const;

private:
    mutable std::mutex m;
    std::map<const ccl_device_driver*, context_array_t> drivers_context;
};
using ccl_driver_context_ptr = std::shared_ptr<ccl_context>;
} // namespace native
