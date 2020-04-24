#pragma once

#include "common/comm/l0/communicator/base_communicator.hpp"

namespace native
{
    template<ccl::device_topology_type schema_id>
    struct device_community;
}

template<ccl::device_topology_type topology>
class typed_base_communicator : public base_communicator
{
public:
    using base_t = base_communicator;
    using self_t = typed_base_communicator<topology>;

    // Topologies
    static constexpr ccl::device_topology_type topology_type()
    {
        return topology;
    }

    static constexpr ccl::device_topology_class topology_class()
    {
        return ccl::topology_to_class<topology>();
    }

    typed_base_communicator(ccl::unified_device_type&& device,
                            size_t thread_idx, size_t process_idx,
                            const ccl::shared_comm_device_attr_t& attr);

    ccl::device_topology_type get_topology_type() const override;

    void initialize_comm_addr(const ccl::device_index_type& device_id,
                              std::shared_ptr<native::device_community<topology>> community);

    bool is_ready() const override;

    // Device community interface
    template<class device_t>
    size_t get_device_count() const;

    template<class device_t>
    native::indexed_device_container<device_t>& get_devices();

    // troubleshooting
    std::string to_string() const;

    std::shared_ptr<native::device_community<topology>> device_community_impl;
};

/* Helpers */
#define ALLREDUCE_EXPLICIT_INSTANTIATION(communicator_class, buffer_type)                                                       \
template ccl::device_communicator::coll_request_t communicator_class::allreduce_impl(const buffer_type* send_buf,                  \
                                                                                  buffer_type* recv_buf,                        \
                                                                                  size_t count,                                 \
                                                                                  ccl::reduction reduction,                     \
                                                                                  const ccl::coll_attr* attr,                   \
                                                                                  ccl::stream::impl_t& stream);
