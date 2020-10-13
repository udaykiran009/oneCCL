#pragma once

namespace native {
namespace detail {

/**
 *
 */
template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
struct rank_getter {
    rank_getter(const ccl::device_index_type& device_idx,
                std::multiset<ccl::device_index_type>& registered_ids);

    template <class device_t>
    void operator()(const native::indexed_device_container<device_t>& container);

    template <class device_t>
    void operator()(const native::plain_device_container<device_t>& container);

    size_t get_assigned_rank() const;
    size_t get_assigned_size() const;

private:
    ccl::device_index_type device_id;
    std::multiset<ccl::device_index_type>& registered_device_id;
    size_t rank = 0;
    size_t size = 0;
    bool find = false;
    size_t enumerator = 0;
};

/**
 *
 */
template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
rank_getter<group_id, class_id>::rank_getter(const ccl::device_index_type& device_idx,
                                             std::multiset<ccl::device_index_type>& registered_ids)
        : device_id(device_idx),
          registered_device_id(registered_ids) {}

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
template <class device_t>
void rank_getter<group_id, class_id>::operator()(
    const native::indexed_device_container<device_t>& container) {
    if (find) {
        return;
    }

    for (const auto& dev : container) {
        ccl_device& device = dev.second->get_device();
        const ccl::device_index_type& find_id = device.get_device_path();
        if (find_id == device_id) {
            if (enumerator == registered_device_id.count(device_id)) {
                rank = dev.first; //dev.second->template get_comm_data<group_id>().rank;
                //size = dev.second->template get_comm_data<group_id, class_id>().size;
                find = true;

                registered_device_id.insert(device_id);
            }
            enumerator++;
        }

        if (find) {
            return;
        }
    }
}

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
template <class device_t>
void rank_getter<group_id, class_id>::operator()(
    const native::plain_device_container<device_t>& container) {
    if (find) {
        return;
    }

    for (const auto& dev : container) {
        ccl_device& device = dev.second->get_device();
        ccl::device_index_type find_id = device.get_device_path();
        if (find_id == device_id) {
            if (enumerator == registered_device_id.count(device_id)) {
                rank = dev.second->template get_comm_data<group_id, class_id>().rank;
                //size = dev.second->template get_comm_data<group_id, class_id>().size;
                find = true;

                registered_device_id.insert(device_id);
            }
            enumerator++;
        }
        if (find) {
            return;
        }
    }
}

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
size_t rank_getter<group_id, class_id>::get_assigned_rank() const {
    if (!find) {
        throw std::runtime_error(
            std::string(__FUNCTION__) +
            "rank_getter doesn't assign rank for device: " + ccl::to_string(device_id));
    }
    return rank;
}
} // namespace detail
} // namespace native
