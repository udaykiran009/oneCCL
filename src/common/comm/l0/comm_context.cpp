#include "ccl_aliases.hpp"

#include "common/comm/l0/comm_context_impl.hpp"
#include "common/utils/spinlock.hpp"
#include "common/comm/atl_tag.hpp"

namespace ccl
{
comm_group::comm_group(shared_communicator_t parent_comm,
                                    size_t threads_count, size_t on_process_ranks_count):
    pimpl(new gpu_comm_attr(parent_comm, threads_count, on_process_ranks_count))
{
};

bool comm_group::sync_group_size(size_t device_group_size)
{
    return pimpl->sync_group_size(device_group_size);
}

comm_group::~comm_group()
{
}


/***********************************************************************/

#define COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(type)                                         \
template std::vector<ccl::device_communicator>                                                          \
CCL_API ccl::comm_group::create_communicators(const type& device_ids,                              \
                                              ccl::device_comm_split_attr_t attr);


// container-based method force-instantiation will trigger ALL other methods instantiations
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(vector_class<ccl::device_index_type>);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(list_class<ccl::device_index_type>);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(ccl::device_indices_t);
#ifdef CCL_ENABLE_SYCL
    COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(cl::sycl::vector_class<cl::sycl::device>);
#endif

}
