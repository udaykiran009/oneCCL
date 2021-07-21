#include "oneapi/ccl/config.h"

#if defined(CCL_ENABLE_SYCL) && defined(MULTI_GPU_SUPPORT)

#include "ze_handle_manager.hpp"

void ze_handle_manager::init(const ccl_stream* stream) {
    auto sycl_device = stream->get_native_stream().get_device();
    auto sycl_context = stream->get_native_stream().get_context();

    device = sycl_device.template get_native<cl::sycl::backend::level_zero>();
    context = sycl_context.template get_native<cl::sycl::backend::level_zero>();

    CCL_THROW_IF_NOT(device != nullptr, "device is not valid");
    CCL_THROW_IF_NOT(context != nullptr, "context is not valid");

    LOG_DEBUG("initialization of ze_handle_manager is completed");
}

void ze_handle_manager::clear() {
    for (int rank = 0; rank < handles.size(); rank++) {
        for (size_t buf_idx = 0; buf_idx < handles[rank].size(); buf_idx++) {
            const auto& handle_info = handles[rank][buf_idx];
            ze_ipc_mem_handle_t handle = handle_info.handle;
            void* mem_ptr = handles[rank][buf_idx].mem_ptr;
            size_t mem_offset = handle_info.mem_offset;

            LOG_DEBUG("clear: handle mem_ptr: ",
                      handles[rank][buf_idx].mem_ptr,
                      ", handle fd: ",
                      *(int*)handles[rank][buf_idx].handle.data,
                      ", rank: ",
                      rank,
                      ", buf_idx: ",
                      buf_idx);
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
                LOG_INFO("unable to close memory handle: ",
                         ret,
                         ", for rank: ",
                         rank,
                         ", buf_idx: ",
                         buf_idx);
            }

            // TODO: remove, when the fix arrives from L0 side
            int fd;
            memcpy(&fd, handle.data, sizeof(fd));
            close(fd);
        }
    }

    if (!handles.empty()) {
        LOG_DEBUG("handles are cleared successfully");
    }

    handles.clear();
}

void ze_handle_manager::set(const std::vector<std::vector<ipc_handle_info>>& handles_arg) {
    CCL_THROW_IF_NOT(!handles_arg.empty(), "handles_arg argument is empty");
    CCL_THROW_IF_NOT(handles.empty(), "handles should be empty before set");

    handles = handles_arg;
    LOG_DEBUG("handles are set successfully, size of handles: ", handles.size());
}

void ze_handle_manager::get(const int rank, const size_t buf_idx, ccl_buffer& buf) {
    ze_result_t ret = ZE_RESULT_SUCCESS;

    CCL_THROW_IF_NOT(rank < handles.size(), "rank is not valid value: ", rank);
    CCL_THROW_IF_NOT(buf_idx < handles[rank].size(), "buf_idx is not valid value: ", buf_idx);

    ze_ipc_mem_handle_t handle = handles[rank][buf_idx].handle;
    void* mem_ptr = handles[rank][buf_idx].mem_ptr;

    LOG_DEBUG("context: ", context, ", device: ", device, ", rank: ", rank, ", buf_idx: ", buf_idx);
    if (mem_ptr == nullptr) {
        ret = zeMemOpenIpcHandle(context, device, handle, 0 /*cache allocation*/, &mem_ptr);
        if (ret) {
            CCL_THROW(
                "unable to open handle: ", ret, ", for rank: ", rank, ", for buf_idx: ", buf_idx);
        }
    }
    LOG_DEBUG("get: handle mem_ptr: ",
              mem_ptr,
              ", handle fd: ",
              *(int*)handle.data,
              ", for rank: ",
              rank,
              ", for buf_idx: ",
              buf_idx);

    size_t mem_offset = handles[rank][buf_idx].mem_offset;
    // add offset that we received along with the handle
    mem_ptr = static_cast<void*>(static_cast<char*>(mem_ptr) + mem_offset);

    buf.set(mem_ptr);
}
#endif // CCL_ENABLE_SYCL && MULTI_GPU_SUPPORT