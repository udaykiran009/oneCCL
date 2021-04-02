#pragma once
#include <atomic>
#include <map>
#include <memory>
#include "common/comm/l0/context/scale/base/base_session_key.hpp"
#include "common/comm/l0/modules/supported_modules.hpp"

namespace native {
namespace observer {

/* Low levels session
 * contains raw data for net operations
 */
class base_session {
public:
    virtual void prepare(size_t observer_domain_index,
                         size_t observer_domain_count,
                         void* type_erased_param) = 0;

protected:
    virtual ~base_session() = default;
};

struct session_notification {
    session_notification(void* addr, size_t size_bytes)
            : host_src_ptr(addr),
              src_size_bytes(size_bytes) {}
    void* host_src_ptr;
    size_t src_size_bytes;
};

using shared_session_ptr_t = std::shared_ptr<base_session>;

} // namespace observer
} // namespace native