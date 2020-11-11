#pragma once
#include "oneapi/ccl/types.hpp"
#include "oneapi/ccl/comm_split_attr_ids.hpp"
#include "common/comm/l0/device_group_routing_schema.hpp"
#include "common/comm/l0/devices/devices_declaration.hpp"
#include "common/comm/l0/gpu_device_types.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/device_community_utils.hpp"

namespace native {

template <ccl::device_topology_type schema_id>
struct device_community {
    device_community(const ccl::context_comm_addr& comm_addr)
            : community_addr(comm_addr),
              devices(new specific_indexed_device_storage()) {}

     device_rank_table_t registered_device_id;

    specific_indexed_device_storage& get_device_storage() {
        auto& ptr = get_impl();
        if (!ptr) {
            abort();
        }
        return *ptr;
    }

    template <class device_t>
    indexed_device_container<device_t>& get_devices() {
        static native::indexed_device_container<device_t> empty;

        return devices ? std::get<device_t::type_idx()>(*devices) : empty;
    }

    template <class device_t>
    size_t get_device_count() const {
        return devices ? std::get<device_t::type_idx()>(*devices).size() : 0;
    }

    template <ccl::group_split_type group_id, class ...DeviceTypes>
    void bind_device_by_id(const ccl::device_index_type& device_id,
                           ccl::context_comm_addr& registered_addr,
                           device_variant_t<DeviceTypes...>& out_binder,
                           size_t preferred_rank)
    {
        if (!get_impl()) {
            std::string err_str;
            {
                std::stringstream str;
                ccl_logger::format(str,
                                   "Cannot initialize comm_addr for device id: ",
                                   device_id,
                                   " on topology: ",
                                   ::to_string(group_id),
                                   ", class: ",
                                   ::to_string(schema_id),
                                   ", empty device storage has got from context");
                err_str = str.str();
            }
            LOG_ERROR(err_str);
            throw std::runtime_error(err_str);
        }

        if (registered_addr.comm_rank != 0 or registered_addr.comm_size != 0) {
            std::string err_str;
            {
                std::stringstream str;
                ccl_logger::format(str,
                                   "Cannot register_device_by_id in topology for device id: ",
                                   device_id,
                                   " on topology: ",
                                   ::to_string(group_id),
                                   ", class: ",
                                   ::to_string(schema_id),
                                   ", because topology registered already, comm addr:",
                                   registered_addr.to_string());
                err_str = str.str();
            }
            LOG_ERROR(err_str);
            throw std::runtime_error(err_str);
        }

        // find device in topology and obtain its rank/sie
        detail::rank_binder<group_id, schema_id> initializer(device_id, get_device_storage(), registered_device_id, preferred_rank);
        ccl_tuple_for_each(out_binder, initializer);

        // copy shared data from community addr
        registered_addr = community_addr;

        // get individual rank from initializer
        registered_addr.comm_rank = initializer.get_assigned_rank();
    }

    const ccl::context_comm_addr& get_comm_addr() const noexcept {
        return community_addr;
    }

    template <ccl::group_split_type group_id>
    std::string to_string() const {
        std::stringstream result;
        result << "Topology: " << ::to_string(schema_id) << "\n";
        native::detail::printer<group_id, schema_id> p;
        if (devices) {
            ccl_tuple_for_each(*devices, p);
            result << p.to_string();
        }
        else {
            result << "EMPTY";
        }
        return result.str();
    }

private:
    ccl::context_comm_addr community_addr;
    std::unique_ptr<specific_indexed_device_storage>& get_impl() {
        return devices;
    }

    std::unique_ptr<specific_indexed_device_storage> devices;
};
} // namespace native
