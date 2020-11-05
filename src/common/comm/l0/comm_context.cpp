#include "oneapi/ccl/aliases.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#include "common/comm/l0/comm_context_impl.hpp"
#include "common/utils/spinlock.hpp"
#include "common/comm/atl_tag.hpp"

namespace ccl {
comm_group::comm_group(shared_communicator_t parent_comm,
                       size_t threads_per_process,
                       size_t ranks_per_process,
                       group_unique_key id)
        : pimpl(new gpu_comm_attr(parent_comm, threads_per_process, ranks_per_process, id)){};

bool comm_group::sync_group_size(size_t device_group_size) {
    return pimpl->sync_group_size(device_group_size);
}

comm_group::~comm_group() {}

const group_unique_key& comm_group::get_unique_id() const {
    return pimpl->get_unique_id();
}
/*
std::string comm_group::to_string() const
{
    pimpl->to_string();
}*/
} // namespace ccl
// container-based method force-instantiation will trigger ALL other methods instantiations
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(ccl::vector_class<ccl::device_index_type>,
                                             typename ccl::unified_context_type::ccl_native_t);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(ccl::list_class<ccl::device_index_type>,
                                             typename ccl::unified_context_type::ccl_native_t);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(ccl::device_indices_type,
                                             typename ccl::unified_context_type::ccl_native_t);
COMM_CREATOR_INDEXED_INSTANTIATION_TYPE(ccl::device_index_type,
                                        typename ccl::unified_context_type::ccl_native_t);

COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(
    ccl::vector_class<typename ccl::unified_device_type::ccl_native_t>,
    typename ccl::unified_context_type::ccl_native_t);
COMM_CREATOR_INDEXED_INSTANTIATION_TYPE(typename ccl::unified_device_type::ccl_native_t,
                                        typename ccl::unified_context_type::ccl_native_t);
