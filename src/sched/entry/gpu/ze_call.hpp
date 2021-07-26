#pragma once

#include "common/log/log.hpp"

#include <ze_api.h>

namespace ccl {
namespace ze {

// rule level zero calls serialization
enum serialize_mode {
    ze_serialize_none = 0, // no locking or blocking
    ze_serialize_lock = 1, // locking around each ZE_CALL
    ze_serialize_block = 2, // blocking ZE calls
};

// class provides the serialization of level zero calls
class ze_call {
private:
    // mutex that is used for total serialization
    static std::mutex mutex;

public:
    ze_call();
    ~ze_call();
    void do_call(ze_result_t ze_result, const char* ze_name);
};

//host synchronize primitives
template <typename T>
ze_result_t zeHostSynchronize(T handle);
template <typename T, typename Func>
ze_result_t zeHostSynchronizeImpl(Func sync_func, T handle) {
    return sync_func(handle, std::numeric_limits<uint64_t>::max());
}

} // namespace ze
} // namespace ccl