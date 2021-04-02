#include <sstream>

#include "common/comm/l0/context/scale/numa/numa_session.hpp"
#include "common/log/log.hpp"

namespace native {
namespace observer {

numa_session_iface::numa_session_iface(session_key key) : sess_key(key) {}

size_t numa_session_iface::get_send_tag() const {
    return send_tag;
}

const session_key& numa_session_iface::get_session_key() const {
    return sess_key;
}

std::string numa_session_iface::to_string() const {
    std::stringstream ss;
    ss << "session key identifier: " << get_session_key();
    return ss.str();
}

} // namespace observer
} // namespace native
