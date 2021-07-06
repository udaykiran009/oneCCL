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

class ze_handle_manager {
public:
    ze_handle_manager() {}

    ~ze_handle_manager() {
        clear();
    }

    void init(ccl_stream* stream) {
        auto sycl_device = stream->get_native_stream().get_device();
        auto sycl_context = stream->get_native_stream().get_context();

        device = sycl_device.template get_native<cl::sycl::backend::level_zero>();
        context = sycl_context.template get_native<cl::sycl::backend::level_zero>();
        LOG_DEBUG("initialization of ze_handle_manager is completed");
    }

    void clear() {
        int res = 0;
        ze_result_t ret = ZE_RESULT_SUCCESS;
        for (auto pair_idx : opened_mem_handles) {
            int fd;
            void* handle_ptr = pair_idx.first;
            ze_ipc_mem_handle_t handle = pair_idx.second;

            ret = zeMemCloseIpcHandle(context, handle_ptr);
            if (ret != ZE_RESULT_SUCCESS) {
                CCL_FATAL("unable to close memory handle: ", ret);
            }

            memcpy(&fd, handle.data, sizeof(fd));
            res = close(fd);
            if (res) {
                CCL_FATAL("unable to close fd of a handle: ", res);
            }
        }
        LOG_DEBUG("handles are cleared successfully");
    }

    void set(std::vector<std::vector<ze_ipc_mem_handle_t>>& handles_arg) {
        CCL_THROW_IF_NOT(!handles_arg.empty(), "handles argument is empty");
        handles = handles_arg;
        LOG_DEBUG("handles are set successfully");
    }

    void get(const int rank, const size_t buf_idx, ccl_buffer& buf) {
        void* memory = nullptr;
        ze_result_t ret = ZE_RESULT_SUCCESS;

        CCL_THROW_IF_NOT(rank < handles.size(), "rank is not valid value: ", rank);
        CCL_THROW_IF_NOT(buf_idx < handles[rank].size(), "buf_idx is not valid value: ", buf_idx);

        ze_ipc_mem_handle_t handle = handles[rank][buf_idx];

        LOG_DEBUG("context: ", context, "device: ", device, "rank: ", rank, "buf_idx: ", buf_idx);
        ret = zeMemOpenIpcHandle(context, device, handle, 0 /*cache allocation*/, &memory);
        if (ret) {
            CCL_THROW("unable to open memory handle: ", ret);
        }
        LOG_DEBUG("open memory: ", memory);

        opened_mem_handles.push_back(std::make_pair(memory, handle));
        buf.set(memory);
    }

private:
    ze_context_handle_t context;
    ze_device_handle_t device;
    std::vector<std::vector<ze_ipc_mem_handle_t>> handles;
    std::vector<std::pair<void*, ze_ipc_mem_handle_t>> opened_mem_handles;
};
#endif /* CCL_ENABLE_SYCL and MULTI_GPU_SUPPORT */
