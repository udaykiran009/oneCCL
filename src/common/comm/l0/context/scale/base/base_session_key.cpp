#include "common/comm/l0/context/scale/base/base_session_key.hpp"

namespace native {
namespace observer {

bool session_key::operator<(const session_key& other) const noexcept {
    return hash < other.hash;
}

std::string session_key::to_string() const {
    return std::to_string(hash);
}

} // namespace observer
} // namespace native
