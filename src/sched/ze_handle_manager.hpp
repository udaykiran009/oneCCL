#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) and defined(MULTI_GPU_SUPPORT)
#include <iostream>
#include <limits>
#include <string>
#include <vector>
#include <utility>

#include "common/stream/stream.hpp"

#include <ze_api.h>
#include <CL/sycl/backend/level_zero.hpp>

struct ipc_handle_info {
    ze_ipc_mem_handle_t handle;
    size_t mem_offset;
};

class ze_handle_manager {
public:
    ze_handle_manager() {}

    ~ze_handle_manager() {
        // TODO: fix cleanup. it throw when close fd
        // clear();
    }

    void init(const ccl_stream* stream) {
        auto sycl_device = stream->get_native_stream().get_device();
        auto sycl_context = stream->get_native_stream().get_context();

        device = sycl_device.template get_native<cl::sycl::backend::level_zero>();
        context = sycl_context.template get_native<cl::sycl::backend::level_zero>();
        LOG_DEBUG("initialization of ze_handle_manager is completed");
    }

    void clear() {
        int res = 0;
        for (const auto& handle_pack : opened_handles) {
            int fd;
            void* mem_ptr = handle_pack.first;
            const auto& ipc_info = handle_pack.second;

            ze_ipc_mem_handle_t handle = ipc_info.handle;
            size_t mem_offset = ipc_info.mem_offset;

            // when closing the handle we need to take care of pointers that points to the
            // same level zero allocation. They're simply offsetted from some base pointer
            // although represented by different FDs. If we close this base pointer,
            // all the derived pointers are closed(unmapped) as well. To handle this case
            // we ignore the result of close function which would fail if we close a pointer
            // which is already closed. The function has semantic of free() call, so the result
            // is not much useful anyway.
            void* base_alloc_ptr = static_cast<char*>(mem_ptr) - mem_offset;

            auto ret = zeMemCloseIpcHandle(context, base_alloc_ptr);
            if (ret != ZE_RESULT_SUCCESS) {
                LOG_WARN("unable to close memory handle: ", ret);
            }

            memcpy(&fd, handle.data, sizeof(fd));
            res = close(fd);
            if (res) {
                CCL_FATAL("unable to close fd of a handle: ", res);
            }
        }
        LOG_DEBUG("handles are cleared successfully");
    }

    void set(const std::vector<std::vector<ipc_handle_info>>& handles_arg) {
        CCL_THROW_IF_NOT(!handles_arg.empty(), "handles argument is empty");
        handles = handles_arg;
        LOG_DEBUG("handles are set successfully");
    }

    void get(const int rank, const size_t buf_idx, ccl_buffer& buf) {
        void* memory = nullptr;
        ze_result_t ret = ZE_RESULT_SUCCESS;

        CCL_THROW_IF_NOT(rank < handles.size(), "rank is not valid value: ", rank);
        CCL_THROW_IF_NOT(buf_idx < handles[rank].size(), "buf_idx is not valid value: ", buf_idx);

        ze_ipc_mem_handle_t handle = handles[rank][buf_idx].handle;

        LOG_DEBUG("context: ", context, "device: ", device, "rank: ", rank, "buf_idx: ", buf_idx);
        ret = zeMemOpenIpcHandle(context, device, handle, 0 /*cache allocation*/, &memory);
        if (ret) {
            CCL_THROW("unable to open memory handle: ", ret);
        }
        LOG_DEBUG("open memory: ", memory);

        size_t mem_offset = handles[rank][buf_idx].mem_offset;
        // add offset that we received along with the handle
        memory = static_cast<void*>(static_cast<char*>(memory) + mem_offset);

        opened_handles.push_back({ memory, { handle, mem_offset } });
        buf.set(memory);
    }

private:
    ze_context_handle_t context;
    ze_device_handle_t device;
    std::vector<std::vector<ipc_handle_info>> handles;
    std::vector<std::pair<void*, ipc_handle_info>> opened_handles;
};
#endif /* CCL_ENABLE_SYCL and MULTI_GPU_SUPPORT */
