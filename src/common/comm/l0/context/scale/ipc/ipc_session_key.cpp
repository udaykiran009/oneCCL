#include "common/comm/l0/context/scale/ipc/ipc_session_key.hpp"

namespace native {

bool ipc_session_key::operator<(const ipc_session_key& other) const noexcept {
    return hash < other.hash;
}

std::string ipc_session_key::to_string() const {
    return std::to_string(hash);
}

} // namespace native
