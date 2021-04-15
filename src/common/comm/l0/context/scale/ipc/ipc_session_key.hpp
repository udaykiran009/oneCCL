#pragma once
#include <functional>
#include <string>
#include <vector>

#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "coll/algorithms/algorithms_enum.hpp"
#include "coll/coll_param.hpp"

namespace native {

template <ccl_coll_type type>
struct ipc_invoke_params {
    ipc_invoke_params(std::vector<ccl_device::device_ipc_memory_handle>&& h,
                      const coll_param_gpu& params)
            : handles(std::move(h)),
              params{ params } {}

    static constexpr ccl_coll_type get_coll_type() {
        return type;
    }

    const coll_param_gpu& get_kernel_params() const {
        return params;
    }

    std::vector<ccl_device::device_ipc_memory_handle> handles;
    // TODO: can we guarantee that this object is not destroyed before l0 entry and
    // use const& here?
    coll_param_gpu params;
};

struct ipc_session_key {
    using hash_core_t = size_t;

    friend std::ostream& operator<<(std::ostream& out, const ipc_session_key& key) {
        out << key.to_string();
        return out;
    }

    template <class T>
    ipc_session_key(const T* src) : hash(std::hash<const T*>{}(src)) {}

    bool operator<(const ipc_session_key& other) const noexcept;

    std::string to_string() const;

private:
    hash_core_t hash;
};
} // namespace native
