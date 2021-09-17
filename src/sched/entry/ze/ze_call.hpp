#pragma once

#include <ze_api.h>

namespace ccl {
namespace ze {

// class provides the serialization of level zero calls
class ze_call {
public:
    // rule level zero calls serialization
    enum serialize_mode : int {
        none, // no locking or blocking
        lock, // locking around each ZE_CALL
        block, // blocking ZE calls
    };

    ze_call();
    ~ze_call();
    ze_result_t do_call(ze_result_t ze_result, const char* ze_name) const;

private:
    // mutex that is used for total serialization
    static std::mutex mutex;
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
