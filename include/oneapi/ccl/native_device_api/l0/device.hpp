#pragma once
#include <mutex> //TODO use shared

#include "oneapi/ccl/native_device_api/l0/primitives.hpp"
#include "oneapi/ccl/native_device_api/l0/utils.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"
#include "oneapi/ccl/native_device_api/l0/driver.hpp"

namespace native {
struct ccl_device_platform;
struct ccl_device_driver;
struct ccl_subdevice;
struct ccl_device;
struct ccl_context;

detail::cross_device_rating property_p2p_rating_calculator(const ccl_device& lhs,
                                                           const ccl_device& rhs,
                                                           size_t weight);

// TODO not thread-safe!!!
struct ccl_device : public cl_base<ze_device_handle_t, ccl_device_driver, ccl_context_holder>,
                    std::enable_shared_from_this<ccl_device> {
    friend std::ostream& operator<<(std::ostream&, const ccl_device&);
    friend struct ccl_subdevice;
    using base = cl_base<ze_device_handle_t, ccl_device_driver, ccl_context_holder>;
    using handle_t = base::handle_t;
    using base::handle;
    using base::owner_t;
    using base::owner_ptr_t;
    using base::context_t;
    using base::context_ptr_t;
    using context_storage_type = std::shared_ptr<ccl_context_holder>;

    using subdevice_ptr = std::shared_ptr<ccl_subdevice>;
    using const_subdevice_ptr = std::shared_ptr<const ccl_subdevice>;
    using sub_devices_container_type = std::map<ccl::index_type, subdevice_ptr>;

    template <class elem_t = uint8_t>
    using device_memory = memory<elem_t, ccl_device, ccl_context>;
    template <class elem_t = uint8_t>
    using device_memory_ptr = std::shared_ptr<memory<elem_t, ccl_device, ccl_context>>;

    using device_ipc_memory = ipc_memory<ccl_device, ccl_context>;
    using device_ipc_memory_handle = ipc_memory_handle<ccl_device, ccl_context>;

    using queue_group_properties = std::vector<ze_command_queue_group_properties_t>;
    using device_queue = queue<ccl_device, ccl_context>;

    using device_queue_fence = queue_fence<ccl_device, ccl_context>;
    using device_cmd_list = cmd_list<ccl_device, ccl_context>;
    using device_module = module<ccl_device, ccl_context>;
    using device_module_ptr = std::shared_ptr<device_module>;
    using device_event = event;
    using indexed_handles = indexed_storage<handle_t>;

    ccl_device(handle_t h, owner_ptr_t&& parent, std::weak_ptr<ccl_context_holder>&& ctx);
    ccl_device(const ccl_device& src) = delete;
    ccl_device& operator=(const ccl_device& src) = delete;
    virtual ~ccl_device();

    static const ze_command_queue_desc_t& get_default_queue_desc();
    static const ze_command_list_desc_t& get_default_list_desc();
    static const ze_device_mem_alloc_desc_t& get_default_mem_alloc_desc();
    static const ze_host_mem_alloc_desc_t& get_default_host_alloc_desc();

    static indexed_handles get_handles(
        const ccl_device_driver& driver,
        const ccl::device_indices_type& indexes = ccl::device_indices_type());
    static std::shared_ptr<ccl_device> create(
        handle_t h,
        owner_ptr_t&& driver,
        const ccl::device_indices_type& indexes = ccl::device_indices_type());

    std::shared_ptr<ccl_device> get_ptr() {
        return this->shared_from_this();
    }

    std::shared_ptr<const ccl_device> get_ptr() const {
        return this->shared_from_this();
    }

    context_storage_type get_contexts();
    sub_devices_container_type& get_subdevices();
    const sub_devices_container_type& get_subdevices() const;

    subdevice_ptr get_subdevice(const ccl::device_index_type& path);
    const_subdevice_ptr get_subdevice(const ccl::device_index_type& path) const;

    // properties
    virtual bool is_subdevice() const noexcept;
    virtual ccl::index_type get_device_id() const;
    virtual ccl::device_index_type get_device_path() const;

    const ze_device_properties_t& get_device_properties() const;
    const ze_device_compute_properties_t& get_compute_properties() const;
    ze_device_p2p_properties_t get_p2p_properties(const ccl_device& remote_device) const;

    // primitives creation
    template <class elem_t>
    device_memory<elem_t> alloc_memory(
        size_t count,
        size_t alignment,
        std::shared_ptr<ccl_context> ctx,
        const ze_device_mem_alloc_desc_t& mem_descr = get_default_mem_alloc_desc(),
        const ze_host_mem_alloc_desc_t& host_descr = get_default_host_alloc_desc()) {
        return device_memory<elem_t>(
            reinterpret_cast<elem_t*>(
                device_alloc_memory(count * sizeof(elem_t), alignment, mem_descr, host_descr, ctx)),
            count,
            get_ptr(),
            ctx);
    }

    template <class elem_t>
    device_memory_ptr<elem_t> alloc_memory_ptr(
        size_t count,
        size_t alignment,
        std::shared_ptr<ccl_context> ctx,
        const ze_device_mem_alloc_desc_t& mem_descr = get_default_mem_alloc_desc(),
        const ze_host_mem_alloc_desc_t& host_descr = get_default_host_alloc_desc()) {
        return std::make_shared<device_memory<elem_t>>(
            reinterpret_cast<elem_t*>(
                device_alloc_memory(count * sizeof(elem_t), alignment, mem_descr, host_descr, ctx)),
            count,
            get_ptr(),
            ctx);
    }

    template <class elem_t>
    device_memory_ptr<elem_t> alloc_shared_memory(
        size_t count,
        size_t alignment,
        const ze_host_mem_alloc_desc_t& host_desc,
        std::shared_ptr<ccl_context> ctx,
        const ze_device_mem_alloc_desc_t& mem_descr = get_default_mem_alloc_desc()) {
        return std::make_shared<device_memory<elem_t>>(
            reinterpret_cast<elem_t*>(device_alloc_shared_memory(
                count * sizeof(elem_t), alignment, host_desc, mem_descr, ctx)),
            count,
            get_ptr(),
            ctx);
    }

    device_ipc_memory_handle create_ipc_memory_handle(void* device_mem_ptr,
                                                      std::shared_ptr<ccl_context> ctx);
    std::shared_ptr<device_ipc_memory_handle> create_shared_ipc_memory_handle(
        void* device_mem_ptr,
        std::shared_ptr<ccl_context> ctx);

    device_ipc_memory get_ipc_memory(std::shared_ptr<device_ipc_memory_handle>&& handle,
                                     std::shared_ptr<ccl_context> ctx);
    std::shared_ptr<device_ipc_memory> restore_shared_ipc_memory(
        std::shared_ptr<device_ipc_memory_handle>&& handle,
        std::shared_ptr<ccl_context> ctx);

    device_queue create_cmd_queue(
        std::shared_ptr<ccl_context> ctx,
        const ze_command_queue_desc_t& properties = get_default_queue_desc());
    device_queue_fence& get_fence(const device_queue& queue, std::shared_ptr<ccl_context> ctx);
    device_queue& get_cmd_queue(const ze_command_queue_desc_t& properties,
                                std::shared_ptr<ccl_context> ctx);
    queue_group_properties get_queue_group_prop() const;

    device_cmd_list create_cmd_list(
        std::shared_ptr<ccl_context> ctx,
        const ze_command_list_desc_t& properties = get_default_list_desc());
    device_cmd_list create_immediate_cmd_list(
        std::shared_ptr<ccl_context> ctx,
        const ze_command_queue_desc_t& properties = get_default_queue_desc());
    device_cmd_list& get_cmd_list(
        std::shared_ptr<ccl_context> ctx,
        const ze_command_list_desc_t& properties = get_default_list_desc());
    device_module_ptr create_module(const ze_module_desc_t& descr,
                                    size_t hash,
                                    std::shared_ptr<ccl_context> ctx);

    template <class elem_t>
    bool is_own_memory(const device_memory<elem_t>& mem_handle) {
        /*if(mem_handle.get_owner() != this)
        {
            //TODO  optimize in release
            //return true;
        }*/

        handle_t assoc_handle =
            get_assoc_device_handle(mem_handle.handle, get_owner(), std::shared_ptr<ccl_context>{});
        return assoc_handle == handle;
    }

    std::string to_string(const std::string& prefix = std::string()) const;

    // primitives ownership release
    void on_delete(ze_command_queue_handle_t& handle, ze_context_handle_t& ctx);
    void on_delete(ze_command_list_handle_t& handle, ze_context_handle_t& ctx);
    void on_delete(ze_device_handle_t& sub_device_handle, ze_context_handle_t& ctx);
    void on_delete(ze_ipc_mem_handle_t& ipc_mem_handle, ze_context_handle_t& ctx);
    void on_delete(ip_memory_elem_t& ipc_mem, ze_context_handle_t& ctx);
    void on_delete(ze_module_handle_t& module_handle, ze_context_handle_t& ctx);
    template <class elem_t>
    void on_delete(elem_t* mem_handle, ze_context_handle_t& ctx) {
        device_free_memory(static_cast<void*>(mem_handle), ctx);
    }

    // serialize/deserialize
    static constexpr size_t get_size_for_serialize() {
        return owner_t::get_size_for_serialize();
    }

    static std::weak_ptr<ccl_device> deserialize(
        const uint8_t** data,
        size_t& size,
        std::shared_ptr<ccl_device_platform>& out_platform);
    virtual size_t serialize(std::vector<uint8_t>& out,
                             size_t from_pos,
                             size_t expected_size) const;
    std::shared_ptr<ccl_context> get_default_context();

private:
    ccl_device(handle_t h,
               owner_ptr_t&& parent,
               std::weak_ptr<ccl_context_holder>&& ctx,
               std::false_type);
    void initialize_device_data();
    void* device_alloc_memory(size_t bytes_count,
                              size_t alignment,
                              const ze_device_mem_alloc_desc_t& mem_descr,
                              const ze_host_mem_alloc_desc_t& host_desc,
                              std::shared_ptr<ccl_context> ctx);
    void* device_alloc_shared_memory(size_t bytes_count,
                                     size_t alignment,
                                     const ze_host_mem_alloc_desc_t& host_desc,
                                     const ze_device_mem_alloc_desc_t& mem_descr,
                                     std::shared_ptr<ccl_context> ctx);

    static handle_t get_assoc_device_handle(const void* ptr,
                                            const ccl_device_driver* driver,
                                            std::shared_ptr<ccl_context> ctx);
    void device_free_memory(void* mem_ptr, ze_context_handle_t& ctx);

    //TODO shared mutex?
    std::mutex queue_mutex;
    std::map<ze_command_queue_desc_t, device_queue, command_queue_desc_comparator> cmd_queues;
    std::map<ze_command_queue_handle_t, device_queue_fence> queue_fences;

    std::mutex list_mutex;
    std::map<ze_command_list_desc_t, device_cmd_list, command_list_desc_comparator> cmd_lists;
    sub_devices_container_type sub_devices;

    ze_device_properties_t device_properties;
    std::vector<ze_device_memory_properties_t> memory_properties;
    ze_device_memory_access_properties_t memory_access_properties;
    ze_device_compute_properties_t compute_properties;

    std::map<void*, std::shared_ptr<device_ipc_memory_handle>> ipc_storage;

    std::map<size_t, device_module_ptr> modules;
};
} // namespace native
