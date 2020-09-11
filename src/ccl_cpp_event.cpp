#include "oneapi/ccl/ccl_types.hpp"
#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
#include "event_impl.hpp"


namespace ccl
{

CCL_API event::event(event&& src) :
        base_t(std::move(src))
{
}

CCL_API event::event(impl_value_t&& impl) :
        base_t(std::move(impl))
{
}

CCL_API event::~event()
{
}

CCL_API event& event::operator=(event&& src)
{
    if (src.get_impl() != this->get_impl())
    {
        src.get_impl().swap(this->get_impl());
        src.get_impl().reset();
    }
    return *this;
}

CCL_API void event::build_from_params()
{
    get_impl()->build_from_params();
}
}



#ifdef CCL_ENABLE_SYCL
    API_EVENT_CREATION_FORCE_INSTANTIATION(cl::sycl::event)
    API_EVENT_CREATION_EXT_FORCE_INSTANTIATION(cl_event)
#endif

API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::version, ccl::library_version);
API_EVENT_FORCE_INSTANTIATION_GET(ccl::event_attr_id::native_handle);
API_EVENT_FORCE_INSTANTIATION_GET(ccl::event_attr_id::context)
API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::command_type, uint32_t);
API_EVENT_FORCE_INSTANTIATION(ccl::event_attr_id::command_execution_status, int64_t);

#endif //#if defined(MULTI_GPU_SUPPORT) || defined(CCL_ENABLE_SYCL)
