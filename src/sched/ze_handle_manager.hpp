#pragma once

#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) and defined(MULTI_GPU_SUPPORT)
#include <iostream>
#include <limits>
#include <string>
#include <vector>

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
    }

    void clear() {
        ze_result_t ret = ZE_RESULT_SUCCESS;
        for (size_t handle_idx = 0; handle_idx < opened_mem_handles.size(); handle_idx++) {
            ret = zeMemCloseIpcHandle(context, opened_mem_handles[handle_idx]);
            if (ret != ZE_RESULT_SUCCESS) {
                CCL_FATAL("unable to close memory handle: ", ret, ", handle index:", handle_idx);
            }
        }
    }

    void set(std::vector<std::vector<ze_ipc_mem_handle_t>>& handles_arg) {
        CCL_THROW_IF_NOT(!handles_arg.empty(), "handles argument is empty");
        handles = handles_arg;
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

        opened_mem_handles.push_back(memory);
        buf.set(memory);
    }

private:
    ze_context_handle_t context;
    ze_device_handle_t device;
    std::vector<std::vector<ze_ipc_mem_handle_t>> handles;
    std::vector<void*> opened_mem_handles;
};
#endif /* CCL_ENABLE_SYCL and MULTI_GPU_SUPPORT */