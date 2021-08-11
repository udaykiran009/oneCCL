#pragma once

#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/utils/buffer.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"

#include <ze_api.h>

class ccl_comm;

namespace ccl {

namespace ze {

enum class ipc_mem_type : int { unknown = 0, memory, pool };

std::string to_string(ipc_mem_type type);

struct ipc_handle_info {
    ze_ipc_mem_handle_t handle{};
    size_t offset{};
    void* ptr{};
    ipc_mem_type type{};

    ipc_handle_info();
    ipc_handle_info(const ze_ipc_mem_handle_t& handle, size_t offset, ipc_mem_type type);
    ipc_handle_info& operator=(const ipc_handle_info&) = default;
};

class ipc_handle_manager {
public:
    using mem_handle_map_t = typename std::vector<std::vector<ipc_handle_info>>;

    ipc_handle_manager() = default;
    ipc_handle_manager(const ipc_handle_manager&) = delete;
    ipc_handle_manager& operator=(const ipc_handle_manager&) = delete;
    ~ipc_handle_manager();

    void init(const ccl_comm* comm, const ccl_stream* stream);
    void clear();

    void set(const mem_handle_map_t& handles_arg);
    void get(int rank, size_t buf_idx, ccl_buffer& buf);

    void get_handle(const void* buffer, ze_ipc_mem_handle_t* handle);
    void get_handle(ze_event_pool_handle_t pool, ze_ipc_event_pool_handle_t* handle);
    void open_handle(const ze_ipc_mem_handle_t& handle, void** ptr);
    void open_handle(const ze_ipc_event_pool_handle_t& handle, ze_event_pool_handle_t* pool);

    void get_address_range(const void* ptr, void** base_ptr, size_t* size);

private:
    ze_context_handle_t context{};
    ze_device_handle_t device{};
    ccl_comm* comm{};
    mem_handle_map_t handles;

    size_t get_ptr_diff(const void* ptr1, const void* ptr2) noexcept;
};

} // namespace ze
} // namespace ccl
