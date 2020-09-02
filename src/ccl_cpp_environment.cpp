#include "environment_impl.hpp"


namespace ccl
{

CCL_API ccl::environment::environment()
{
    static auto result = global_data::get().init();
    CCL_CHECK_AND_THROW(result, "failed to initialize CCL");
}

CCL_API ccl::environment::~environment()
{
}

CCL_API ccl::environment& ccl::environment::instance()
{
    static ccl::environment env;
    return env;
}

void CCL_API ccl::environment::set_resize_fn(ccl_resize_fn_t callback)
{
    ccl_status_t result = ccl_set_resize_fn(callback);
    CCL_CHECK_AND_THROW(result, "failed to set resize callback");
    return;
}

ccl_version_t CCL_API ccl::environment::get_version() const
{
    ccl_version_t ret;
    ccl_status_t result = ccl_get_version(&ret);
    CCL_CHECK_AND_THROW(result, "failed to get version");
    return ret;
}
/*
static ccl::stream& get_empty_stream()
{
    static ccl::stream_t empty_stream  = ccl::environment::instance().create_stream();
    return empty_stream;
}
*/

/**
 * Factory methods
 */
// KVS
kvs_t CCL_API environment::create_main_kvs() const
{
    return std::shared_ptr<kvs>(new kvs);
}

kvs_t CCL_API environment::create_kvs(const kvs::addr_t& addr) const
{
    return std::shared_ptr<kvs>(new kvs(addr));
}

//Communicator
communicator CCL_API environment::create_communicator() const
{
    return communicator::create_communicator();
}

communicator CCL_API environment::create_communicator(const size_t size,
                                       shared_ptr_class<kvs_interface> kvs) const
{
    return communicator::create_communicator(size, kvs);
}

communicator CCL_API environment::create_communicator(const size_t size,
                                     const size_t rank,
                                     shared_ptr_class<kvs_interface> kvs) const
{
    return communicator::create_communicator(size, rank, kvs);
}

}



/***************************TypeGenerations*********************************************************/
template <>
ccl::stream CCL_API ccl::environment::create_postponed_api_type<ccl::stream, typename ccl::unified_device_type::ccl_native_t, typename ccl::unified_device_context_type::ccl_native_t>(
                               typename ccl::unified_device_type::ccl_native_t device,
                               typename ccl::unified_device_context_type::ccl_native_t context) const
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    return ccl::stream{stream_provider_dispatcher::create(device, context, ret)};
}
template <>
ccl::stream CCL_API ccl::environment::create_postponed_api_type<ccl::stream, typename ccl::unified_device_type::ccl_native_t>(
                               typename ccl::unified_device_type::ccl_native_t device) const
{
    ccl_version_t ret {};
    ret.major = CCL_MAJOR_VERSION;
    ret.minor = CCL_MINOR_VERSION;
    ret.update = CCL_UPDATE_VERSION;
    ret.product_status = CCL_PRODUCT_STATUS;
    ret.build_date = CCL_PRODUCT_BUILD_DATE;
    ret.full = CCL_PRODUCT_FULL;

    return ccl::stream{stream_provider_dispatcher::create(device, ret)};
}


CREATE_OP_ATTR_INSTANTIATION(ccl::allgatherv_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::allreduce_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::alltoall_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::alltoallv_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::bcast_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::reduce_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::reduce_scatter_attr_t)
CREATE_OP_ATTR_INSTANTIATION(ccl::sparse_allreduce_attr_t)


CREATE_OP_ATTR_INSTANTIATION(ccl::comm_split_attr_t)
#ifdef MULTI_GPU_SUPPORT
    CREATE_OP_ATTR_INSTANTIATION(ccl::device_comm_split_attr_t)
#endif

#ifdef CCL_ENABLE_SYCL
    CREATE_DEV_COMM_INSTANTIATION(cl::sycl::device, cl::sycl::context)
    //-S- TODO CREATE_STREAM_INSTANTIATION(cl::sycl::queue, cl::sycl::context)
    //-S- TODO CREATE_EVENT_INSTANTIATION(cl::sycl::event)
#endif
