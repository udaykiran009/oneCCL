#pragma once

namespace native {

using device_rank_table_t = std::multimap<ccl::device_index_type, size_t /**external rank**/>;

namespace detail {

/**
 *
 */
template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
struct rank_binder {
    rank_binder(const ccl::device_index_type& in_device_idx,
                const native::specific_indexed_device_storage& in_dev_storage,
                device_rank_table_t& out_registered_ids,
                size_t preferred_rank = std::numeric_limits<size_t>::max());

    template <class device_t>
    void operator()(native::device_t_ptr<device_t>& container);

    int get_assigned_rank() const;
    int get_assigned_size() const;

private:
    ccl::device_index_type device_id;
    const native::specific_indexed_device_storage& in_device_storage;
    device_rank_table_t& registered_device_id;
    int rank = 0;
    int size = 0;
    bool find = false;
    size_t enumerator = 0;
    size_t preferred_rank_id;
};

/**
 *
 */

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
rank_binder<group_id, class_id>::rank_binder(const ccl::device_index_type& in_device_idx,
                                             const native::specific_indexed_device_storage& in_dev_storage,
                                             device_rank_table_t& registered_ids,
                                             size_t preferred_rank)
        : device_id(in_device_idx),
          in_device_storage(in_dev_storage),
          registered_device_id(registered_ids),
          preferred_rank_id(preferred_rank) {}

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
template <class device_t>
void rank_binder<group_id, class_id>::operator()(
    native::device_t_ptr<device_t>& comm_device) {
    if (find) {
        return;
    }

    const native::indexed_device_container<device_t>& in_typed_container =
            ccl_tuple_get<native::indexed_device_container<device_t>>(in_device_storage);

    for (const auto& dev : in_typed_container) {
        ccl_device& device = dev.second->get_device();
        const ccl::device_index_type& find_id = device.get_device_path();
        if (find_id == device_id) {
            if (enumerator == registered_device_id.count(device_id)) {
                find = true;

                // set rank for device: automatically or user-preferred
                if (preferred_rank_id == std::numeric_limits<size_t>::max())
                {
                    // automatically from logical topology
                    rank = dev.first;
                }
                else
                {
                    //use user defined rank
                    rank = preferred_rank_id;
                }
                registered_device_id.insert({device_id, rank});

                //bind device
                comm_device = dev.second;
            }
            enumerator++;
        }

        if (find) {
            return;
        }
    }
}

template <ccl::group_split_type group_id, ccl::device_topology_type class_id>
int rank_binder<group_id, class_id>::get_assigned_rank() const {
    if (!find) {
        throw std::runtime_error(
            std::string(__FUNCTION__) +
            "rank_binder doesn't assign rank for device: " + ccl::to_string(device_id));
    }
    return rank;
}
} // namespace detail
} // namespace native
