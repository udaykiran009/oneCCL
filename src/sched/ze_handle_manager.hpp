#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)

#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <utility>

#include <ze_api.h>
#include <CL/sycl/backend/level_zero.hpp>

#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/utils/buffer.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"

struct ipc_handle_info {
    ze_ipc_mem_handle_t handle;
    void* ptr;
    size_t offset;

    ipc_handle_info() : ptr(nullptr), offset(0) {
        memset(&handle, 0, sizeof(handle));
    }

    ipc_handle_info(const ze_ipc_mem_handle_t& h, size_t o) {
        handle = h;
        ptr = nullptr;
        offset = o;
    }

    ipc_handle_info& operator=(const ipc_handle_info& other) {
        handle = other.handle;
        ptr = other.ptr;
        offset = other.offset;
        return *this;
    }
};

class ccl_comm;

class ze_handle_manager {
public:
    ze_handle_manager() : context(nullptr), device(nullptr), comm(nullptr) {}

    ~ze_handle_manager() {
        clear();
    }

    void init(const ccl_comm* comm, const ccl_stream* stream);
    void clear();

    void set(const std::vector<std::vector<ipc_handle_info>>& handles_arg);
    void get(const int rank, const size_t buf_idx, ccl_buffer& buf);

private:
    ze_context_handle_t context;
    ze_device_handle_t device;
    ccl_comm* comm;
    std::vector<std::vector<ipc_handle_info>> handles;
};

#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT
