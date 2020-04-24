#pragma once
#include <array>

#include <ze_api.h>

#include "native_device_api/l0/base.hpp"

namespace native
{
std::string to_string(const ze_result_t result);
std::string to_string(ze_memory_type_t type);
std::string to_string(ze_memory_access_capabilities_t cap);
std::string to_string(const ze_device_properties_t& device_properties);
std::string to_string(const ze_device_memory_properties_t& device_mem_properties);
std::string to_string(const ze_device_memory_access_properties_t& mem_access_prop);
std::string to_string(const ze_device_compute_properties_t& compute_properties);
std::string to_string(const ze_memory_allocation_properties_t& prop);
std::string to_string(const ze_device_p2p_properties_t& properties);
std::string to_string(const ze_ipc_mem_handle_t& handle);

/**
 * Specific L0 primitives declaration
 */
template<class resource_owner>
using queue = cl_base<ze_command_queue_handle_t, resource_owner>;

template<class resource_owner>
using cmd_list = cl_base<ze_command_list_handle_t, resource_owner>;

template<class resource_owner>
using module = cl_base<ze_module_handle_t, resource_owner>;

template<class resource_owner>
using ipc_memory_handle = cl_base<ze_ipc_mem_handle_t, resource_owner>;

template<class resource_owner>
using queue_fence = cl_base<ze_fence_handle_t, resource_owner>;

template<class elem_t, class resource_owner,
         class = typename std::enable_if<ccl::is_supported<elem_t>()>::type>
struct memory;

template<class elem_t, class resource_owner>
struct memory<elem_t, resource_owner> : private cl_base<elem_t *, resource_owner>
{
    using base = cl_base<elem_t *, resource_owner>;
    using base::get_owner;
    using base::handle;

    memory(elem_t* h, size_t count, std::weak_ptr<resource_owner>&& owner);

    /**
     *  Memory operations
     */
    // sync memory-copy write
    void enqueue_write_sync(const std::vector<elem_t>& src);
    void enqueue_write_sync(typename std::vector<elem_t>::const_iterator first,
                            typename std::vector<elem_t>::const_iterator last);
    template<int N>
    void enqueue_write_sync(const std::array<elem_t, N>& src);
    void enqueue_write_sync(const elem_t* src, size_t n);

    // async
    queue_fence<resource_owner> enqueue_write_async(const std::vector<elem_t>& src, queue<resource_owner>& queue);
    template<int N>
    queue_fence<resource_owner> enqueue_write_async(const std::array<elem_t, N>& src, queue<resource_owner>& queue);
    queue_fence<resource_owner> enqueue_write_async(const elem_t* src, size_t n, queue<resource_owner>& queue);

    // sync memory-copy read
    std::vector<elem_t> enqueue_read_sync(size_t requested_size = 0) const;

    // async memory-copy
    //TODO
    elem_t* get() noexcept;
    const elem_t* get() const noexcept;

    size_t count() const noexcept;
    size_t size() const noexcept;
private:
    size_t elem_count;
};

struct ip_memory_elem_t
{
    void *pointer = nullptr;
};

template<class resource_owner>
using ipc_memory = cl_base<ip_memory_elem_t, resource_owner>;

std::string get_build_log_string(const ze_module_build_log_handle_t &build_log);
struct command_queue_desc_comparator
{
    bool operator() (const ze_command_queue_desc_t& lhs, const ze_command_queue_desc_t& rhs) const;
};

struct command_list_desc_comparator
{
      bool operator() (const ze_command_list_desc_t& lhs, const ze_command_list_desc_t& rhs) const;
};

}
