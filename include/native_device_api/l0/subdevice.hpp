#pragma once
#include "native_device_api/l0/primitives.hpp"

namespace native
{
struct ccl_device_driver;
struct ccl_device;

// TODO not thread-safe!!!
struct ccl_subdevice : public ccl_device
{
    using base = ccl_device;
    using handle_t = ccl_device::handle_t;
    using owner_ptr_t = std::weak_ptr<ccl_device>;

    using indexed_handles = indexed_storage<handle_t>;

    friend std::ostream& operator<<(std::ostream &, const ccl_subdevice &node);

    ccl_subdevice(handle_t h, owner_ptr_t&& device, base::owner_ptr_t&& driver);
    virtual ~ccl_subdevice();

    // factory
    static indexed_handles get_handles(const ccl_device& device, const ccl::device_indices_t& requested_indices = ccl::device_indices_t());
    static std::shared_ptr<ccl_subdevice> create(handle_t h, owner_ptr_t&& device, base::owner_ptr_t&& driver);

    // properties
    bool is_subdevice() const noexcept override;
    ccl::index_type get_device_id() const override;
    ccl::device_index_type get_device_path() const override;

    // utils
    std::string to_string(const std::string& prefix = std::string()) const;

    // serialize/deserialize
    static constexpr size_t get_size_for_serialize()
    {
        return /*owner_t::get_size_for_serialize()*/sizeof(size_t) + sizeof(device_properties.subdeviceId);
    }
    size_t serialize(std::vector<uint8_t> &out, size_t from_pos, size_t expected_size) const override;
    static std::weak_ptr<ccl_subdevice> deserialize(const uint8_t** data, size_t& size, ccl_device_platform& platform);
private:
    ccl_subdevice(handle_t h, owner_ptr_t&& device, base::owner_ptr_t&& driver, std::false_type);
    void initialize_subdevice_data();
    owner_ptr_t parent_device;
};
}
