//TODO
#if 0
#include <stdexcept>

#include "oneapi/ccl.hpp"
#include "oneapi/ccl/ccl_type_traits.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

#include "common/comm/comm_interface.hpp"
#include "common/comm/host_communicator/host_communicator.hpp"
#include "common/comm/l0/gpu_comm_attr.hpp"
#include "common/comm/l0/device_community.hpp"

#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "oneapi/ccl/native_device_api/compiler_ccl_wrappers_dispatcher.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

std::ostream& operator<<(std::ostream& out, const ccl::device_index_type& index)
{
    out << ccl::to_string(index);
    return out;
}

namespace ccl
{


/* GPU communicator attributes
 */
CCL_API ccl::ccl_device_attr::ccl_device_attr(const ccl::ccl_comm_split_attr& src) :
 base_t(src),
 pimpl(new ccl::device_attr_impl())
{
}

CCL_API ccl::ccl_device_attr::~ccl_device_attr() noexcept
{
}

template<device_comm_split_attr_id attrId,
             class Value,
             typename T>
CCL_API Value ccl::ccl_device_attr::set_value(Value&& v)
{
    return pimpl->set_attribute_value(std::forward<Value>(v));
}

template<device_comm_split_attr_id attrId>
CCL_API const typename ccl::ccl_device_attributes_traits<attrId>::type& ccl::ccl_device_attr::get_value() const
{
    return pimpl->get_attribute_value(
            std::integral_constant<device_comm_split_attr_id, attrId> {});
}

/* Global Environment*/
template<class stream_native_type, typename T>
CCL_API ccl::stream_t ccl::environment::create_stream(stream_native_type& s)
{
    return ccl::stream_t(new ccl::stream(stream_provider_dispatcher::create(s)));
}

CCL_API ccl::comm_group_t ccl::environment::create_comm_group(size_t current_device_group_size, size_t process_device_group_size,
                                                              ccl::shared_communicator_t parent_comm /* = ccl::shared_communicator_t()*/)
{
    if (!parent_comm)
    {
        //use global communicator by default
        ccl::shared_communicator_t(ccl::environment::instance().create_communicator()).swap(parent_comm);
    }

    ccl::comm_group_t group;
    {
        // register group slot in global context table, based on communicator id
        auto host_comm_impl = std::dynamic_pointer_cast<host_communicator>(parent_comm->pimpl);
        if (!host_comm_impl)
        {
            throw ccl::exception(std::string(__FUNCTION__) + " - failed, invalid host communicator type");
        }

        group_context::group_unique_key unique_id =
                    host_comm_impl->get_host_attr()->get_value<ccl_host_color>();

        std::unique_lock<ccl_spinlock> lock(global_ctx.mutex);
        auto ctx_it = global_ctx.communicator_group_map.find(unique_id);
        if(ctx_it == global_ctx.communicator_group_map.end())
        {
            group.reset(new ccl::comm_group(parent_comm,
                                            current_device_group_size,
                                            process_device_group_size));
            global_ctx.communicator_group_map.insert({
                                                        unique_id,
                                                        group
                                                     });
        }
        else
        {
            group = ctx_it->second;
        }
    }

    // sync existing group: blocking operation - wait for all groups
    group->pimpl->sync_group_size(current_device_group_size);
    return group;
}

CCL_API ccl::comm_group::comm_group(ccl::shared_communicator_t parent_comm,
                                    size_t current_device_group_size, size_t process_device_group_size):
    pimpl(new ccl::gpu_comm_attr(parent_comm, current_device_group_size, process_device_group_size))
{
};

/**
 *  Create communicator API:
 */
CCL_API ccl::comm_split_attr ccl::comm_group::create_comm_split_attr()
{
    // TODO
    const auto& host_comm = pimpl->get_host_communicator();
    return ccl::comm_split_attr{new ccl::ccl_device_attr(*(host_comm->get_comm_split_attr()))};
}
/*
 *  Single device communicator creation
 */
template <class DeviceType,
          typename std::enable_if<std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type>
CCL_API ccl::communicator_t ccl::comm_group::create_communicator(const DeviceType& device,
                                                                     ccl::comm_split_attr attr/* = comm_device_attr_t()*/)
{
    LOG_TRACE("Create communicator from device");
    ccl::communicator_interface_ptr impl =
            ccl::communicator_interface::create_communicator_impl(device,
                                                                  pimpl->thread_id,
                                                                  pimpl->ccl_communicator->rank(),
                                                                  attr,
                                                                  pimpl->ccl_communicator->comm_impl.atl);
    // registering device in group - is non blocking operation, until it is not the last device
    pimpl->sync_register_communicator(impl);
    return ccl::communicator_t(new ccl::communicator(impl));
}

template <class DeviceType,
          typename std::enable_if<not std::is_class<typename std::remove_cv<DeviceType>::type>::value,
                                      int>::type>
CCL_API ccl::communicator_t ccl::comm_group::create_communicator(DeviceType device_id,
                                                                    ccl::comm_split_attr attr/* = nullptr*/)
{
    LOG_TRACE("Create communicator from id: ", device_id);

    ccl::communicator_interface_ptr impl = ccl::communicator_interface::create_communicator_impl(device_id,
                                                                                                 pimpl->thread_id,
                                                                                                 pimpl->ccl_communicator->rank(),
                                                                                                 attr);
    // registering device in group - is non blocking operation, until it is not the last device
    pimpl->sync_register_communicator(impl);
    return ccl::communicator_t(new ccl::communicator(impl));
}

/**
 *  Multiple device communicators creation vectorized API implementation
 */
template<class InputIt>
CCL_API std::vector<ccl::communicator_t> ccl::comm_group::create_communicators(InputIt first, InputIt last,
                                                                                   ccl::comm_split_attr attr/* = nullptr*/)
{

    using iterator_value_type = typename std::iterator_traits<InputIt>::value_type;
/*
    using expected_value_type = typename unified_device_type::device_t;
    static_assert(std::is_same<iterator_value_type, expected_value_type>::value,
                  "Not valid InputIt in create_communicators");
*/
    size_t indices_count = std::distance(first, last);
    LOG_TRACE("Create device communicators from index iterators type, count: ", indices_count);

    std::vector<ccl::communicator_t> comms;
    comms.reserve(indices_count);
    std::transform(first, last, std::back_inserter(comms), [this, attr](const iterator_value_type& device_id)
    {
        return create_communicator(device_id, attr);
    });
    return comms;
}

template<template<class...> class Container, class Type>
CCL_API std::vector<ccl::communicator_t> ccl::comm_group::create_communicators(const Container<Type>& device_ids,
                                                                                   ccl::comm_split_attr attr/* = nullptr*/)
{
    //static_assert(std::is_same<Type, ccl::device_index_type>::value, "Invalid Type in create_communicators");
    LOG_TRACE("Create device communicators from index type, count: ", device_ids.size(),
              ". Redirect to iterators version");
    return create_communicators(device_ids.begin(), device_ids.end(), attr);
}

CCL_API ccl::comm_group::device_context_native_const_reference_t ccl::comm_group::get_context() const
{
    //TODO use PIMPL as context provider
    static unified_device_context_type context;
    return context.get();
}


/***********************************************************************/
#define DEVICE_ATTRIBUTE_INSTANTIATION(ATTR_ID, VALUE_TYPE) \
    template VALUE_TYPE CCL_API ccl::ccl_device_attr::set_value<ATTR_ID, VALUE_TYPE>(VALUE_TYPE && \
                                                                                     v); \
    template CCL_API const VALUE_TYPE& ccl::ccl_device_attr::get_value<ATTR_ID>() const;

#define STREAM_CREATOR_INSTANTIATION(type) \
    template ccl::stream_t CCL_API ccl::environment::create_stream(type& stream);

#define COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(type) \
    template std::vector<ccl::communicator_t> CCL_API ccl::comm_group::create_communicators( \
        const type& device_ids, ccl::comm_split_attr attr);

// device attribute instantiations
DEVICE_ATTRIBUTE_INSTANTIATION(ccl_device_preferred_topology_class,
                               typename ccl::ccl_device_attributes_traits<ccl_device_preferred_topology_class>::type);
DEVICE_ATTRIBUTE_INSTANTIATION(ccl_device_preferred_group,
                               typename ccl::ccl_device_attributes_traits<ccl_device_preferred_group>::type);


// stream instantiations
STREAM_CREATOR_INSTANTIATION(ze_command_queue_handle_t)
#ifdef CCL_ENABLE_SYCL
    STREAM_CREATOR_INSTANTIATION(cl::sycl::queue)
#endif

// container-based method force-instantiation will trigger ALL other methods instantiations
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(std::vector<ccl::device_index_type>);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(std::list<ccl::device_index_type>);
COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(ccl::device_indices_t);
#ifdef CCL_ENABLE_SYCL
    COMM_CREATOR_INDEXED_INSTANTIATION_CONTAINER(cl::sycl::vector_class<cl::sycl::device>);
#endif

#endif //TODO
