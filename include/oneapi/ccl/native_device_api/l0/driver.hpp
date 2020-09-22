#pragma once
#include <iostream>
#include <map>

#include "oneapi/ccl/native_device_api/l0/base.hpp"

namespace native {
struct ccl_device_platform;
struct ccl_device;

struct ccl_device_driver : public cl_base<ze_driver_handle_t, ccl_device_platform>,
                           std::enable_shared_from_this<ccl_device_driver> {
    friend std::ostream& operator<<(std::ostream&, const ccl_device_driver&);

    using base = cl_base<ze_driver_handle_t, ccl_device_platform>;
    using handle_t = base::handle_t;
    using device_ptr = std::shared_ptr<ccl_device>;
    using const_device_ptr = std::shared_ptr<const ccl_device>;
    using base::handle;
    using base::owner_ptr_t;

    using base::get;

    using devices_storage_type = std::map<ccl::index_type, device_ptr>;
    using indexed_driver_handles = indexed_storage<handle_t>;

    ccl_device_driver(handle_t h, uint32_t id, owner_ptr_t&& platform);

    static indexed_driver_handles get_handles(
        const ccl::device_indices_t& requested_driver_indexes = ccl::device_indices_t());
    static std::shared_ptr<ccl_device_driver> create(
        handle_t h,
        uint32_t id,
        owner_ptr_t&& platform,
        const ccl::device_mask_t& rank_device_affinity);

    static std::shared_ptr<ccl_device_driver> create(
        handle_t h,
        uint32_t id,
        owner_ptr_t&& platform,
        const ccl::device_indices_t& rank_device_affinity = ccl::device_indices_t());

    std::shared_ptr<ccl_device_driver> get_ptr() {
        return this->shared_from_this();
    }

    uint32_t get_driver_id() const noexcept;

    ze_driver_properties_t get_properties() const;
    const devices_storage_type& get_devices() const noexcept;
    device_ptr get_device(const ccl::device_index_type& path);
    const_device_ptr get_device(const ccl::device_index_type& path) const;

    std::string to_string(const std::string& prefix = std::string()) const;

    // ownership release
    void on_delete(ze_device_handle_t& sub_device_handle);

    // serialize/deserialize
    static constexpr size_t get_size_for_serialize() {
        return sizeof(size_t);
    }
    static std::weak_ptr<ccl_device_driver> deserialize(const uint8_t** data,
                                                        size_t& size,
                                                        ccl_device_platform& driver);
    size_t serialize(std::vector<uint8_t>& out, size_t from_pos, size_t expected_size) const;

    // utility
    static ccl::device_mask_t create_device_mask(const std::string& str_mask,
                                                 std::ios_base::fmtflags flag = std::ios_base::hex);
    static ccl::device_indices_t get_device_indices(const ccl::device_mask_t& mask);
    static ccl::device_mask_t get_device_mask(const ccl::device_indices_t& device_idx);

    uint32_t driver_id;

private:
    devices_storage_type devices;
};
/*
template <class DeviceType,
          typename std::enable_if<not std::is_same<typename std::remove_cv<DeviceType>::type,
                                                   ccl::device_index_type>::value,
                                  int>::type = 0>
ccl_device_driver::device_ptr get_runtime_device(const DeviceType& device);

template <class DeviceType,
          typename std::enable_if<std::is_same<typename std::remove_cv<DeviceType>::type,
                                               ccl::device_index_type>::value,
                                  int>::type = 0>
ccl_device_driver::device_ptr get_runtime_device(DeviceType device);
*/
} // namespace native
