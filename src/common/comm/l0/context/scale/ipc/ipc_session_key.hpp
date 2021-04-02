#pragma once
#include <functional>
#include <string>
#include <vector>

#include "oneapi/ccl/native_device_api/l0/device.hpp"
#include "coll/algorithms/algorithms_enum.hpp"

namespace native {

template <ccl_coll_type type, class kernel_params>
struct ipc_invoke_params {
    using kernel_params_t = kernel_params;

    ipc_invoke_params(std::vector<ccl_device::device_ipc_memory_handle>&& h)
            : handles(std::move(h)) {}

    static constexpr ccl_coll_type get_coll_type() {
        return type;
    }

    std::vector<ccl_device::device_ipc_memory_handle> handles;
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
