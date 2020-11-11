#pragma once
#include <unistd.h>

#include "oneapi/ccl/native_device_api/l0/driver.hpp"
#include "oneapi/ccl/native_device_api/l0/context.hpp"
#include "oneapi/ccl/native_device_api/l0/utils.hpp"

namespace native {
struct ccl_device_platform : std::enable_shared_from_this<ccl_device_platform> {
    using driver_ptr = std::shared_ptr<ccl_device_driver>;
    using const_driver_ptr = std::shared_ptr<ccl_device_driver>;
    using driver_storage_type = std::map<ccl::index_type, driver_ptr>;
    using device_affinity_per_driver = std::map<size_t, ccl::device_mask_t>;
    using context_storage_type = std::shared_ptr<ccl_context_holder>;

    using platform_id_type = size_t;

    //void init_drivers(const device_affinity_per_driver& affinities / * = device_affinity_per_driver()* /);
    void init_drivers(const ccl::device_indices_type& indices = ccl::device_indices_type());

    std::shared_ptr<ccl_device_platform> get_ptr() {
        return this->shared_from_this();
    }

    const_driver_ptr get_driver(ccl::index_type index) const;
    driver_ptr get_driver(ccl::index_type index);

    const driver_storage_type& get_drivers() const noexcept;

    ccl_device_driver::device_ptr get_device(const ccl::device_index_type& path);
    ccl_device_driver::const_device_ptr get_device(const ccl::device_index_type& path) const;

    std::shared_ptr<ccl_context> create_context(std::shared_ptr<ccl_device_driver> driver);
    context_storage_type get_platform_contexts();

    std::string to_string() const;
    void on_delete(ccl_device_driver::handle_t& driver_handle, ze_context_handle_t& ctx);
    void on_delete(ccl_context::handle_t& context, ze_context_handle_t& ctx);

    static std::shared_ptr<ccl_device_platform> create(
        const ccl::device_indices_type& indices = ccl::device_indices_type());
    //static std::shared_ptr<ccl_device_platform> create(const device_affinity_per_driver& affinities);

    detail::adjacency_matrix calculate_device_access_metric(
        const ccl::device_indices_type& indices = ccl::device_indices_type(),
        detail::p2p_rating_function func = detail::binary_p2p_rating_calculator) const;

    // serialize/deserialize
    static constexpr size_t get_size_for_serialize() {
        return sizeof(pid_t) + sizeof(pid_t) + sizeof(platform_id_type);
    }

    static std::weak_ptr<ccl_device_platform> deserialize(const uint8_t** data,
                                                 size_t& size,
                                                 std::shared_ptr<ccl_device_platform>& out_platform);
    size_t serialize(std::vector<uint8_t>& out,
                             size_t from_pos,
                             size_t expected_size) const;

    platform_id_type get_id() const noexcept;
    pid_t get_pid() const noexcept;
private:
    ccl_device_platform(platform_id_type platform_id = 0);

    std::shared_ptr<ccl_device_platform> clone(platform_id_type id, pid_t foreign_pid) const;

    driver_storage_type drivers;
    context_storage_type context;

    platform_id_type id;
    pid_t pid;
};

//extern std::shared_ptr<ccl_device_platform> global_platform;
ccl_device_platform& get_platform();

ccl_device_platform::driver_ptr get_driver(size_t index = 0);
} // namespace native
