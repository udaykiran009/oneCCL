#include "ccl.hpp"
#include "ccl_type_traits.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

#include "common/comm/comm_attributes.hpp"
#include "common/comm/comm_interface.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif

#define CCL_CHECK_AND_THROW(result, diagnostic)   \
  do {                                            \
      if (result != ccl_status_success)           \
      {                                           \
          throw ccl_error(diagnostic);            \
      }                                           \
  } while (0);


namespace ccl
{

static std::atomic<size_t> env_ref_counter;

static ccl::stream_t& get_empty_stream()
{
    static ccl::stream_t empty_stream  = ccl::environment::instance().create_stream();
    return empty_stream;
}

/**
 * Public API interface for host communicator attributes
 */
CCL_API ccl_host_attr::ccl_host_attr(const ccl_host_attr& src) :
        ccl_host_attr(src.get_value<ccl_host_attributes::ccl_host_version>(),
                      *(src.pimpl))
{
}

CCL_API ccl_host_attr::ccl_host_attr(const ccl_version_t& library_version,
                                     const ccl_host_comm_attr_t &core,
                                     ccl_version_t api_version) :
        pimpl(new host_attr_impl(core, library_version))
{
    if (api_version.major != library_version.major)
    {
        throw ccl::ccl_error(std::string("API & library versions are compatible: API is ") +
                             std::to_string(api_version.major) +
                             ", library is: " + std::to_string(library_version.major));
    }
}

CCL_API ccl_host_attr::~ccl_host_attr() noexcept
{
}

template<ccl_host_attributes attrId,
             class Value,
             typename T>
CCL_API Value ccl_host_attr::set_value(Value&& v)
{
    return pimpl->set_attribute_value(std::forward<Value>(v));
}

template<ccl_host_attributes attrId>
CCL_API const typename ccl_host_attributes_traits<attrId>::type& ccl_host_attr::get_value() const
{
    return pimpl->get_attribute_value(
            std::integral_constant<ccl_host_attributes, attrId> {});
}

#define HOST_ATTRIBUTE_INSTANTIATION(ATTR_ID, VALUE_TYPE)                                          \
template                                                                                           \
CCL_API VALUE_TYPE ccl::ccl_host_attr::set_value<ATTR_ID, VALUE_TYPE>(VALUE_TYPE&& v);             \
template                                                                                           \
CCL_API const VALUE_TYPE& ccl::ccl_host_attr::get_value<ATTR_ID>() const;
}

CCL_API ccl::environment::environment()
{
    static auto result = ccl_init();
    env_ref_counter++;
    CCL_CHECK_AND_THROW(result, "failed to initialize ccl");
}

CCL_API ccl::environment& ccl::environment::instance()
{
    static thread_local bool created = false;
    if (!created)
    {
        /*
            environment destructor uses logger for ccl_finalize and it should be destroyed before logger,
            therefore construct thread_local logger at first follows to global/static initialization rules
        */
        LOG_INFO("created environment");
        created = true;
    }
    static thread_local std::unique_ptr<ccl::environment> env(new environment);
    return *env;
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

ccl::comm_attr_t CCL_API ccl::environment::create_host_comm_attr(const ccl_host_comm_attr_t& attr) const
{
    LOG_TRACE("Create host attributes");
    return ccl::comm_attr_t(new ccl::ccl_host_attr(get_version(), attr));
}

ccl::communicator_t CCL_API ccl::environment::create_communicator(const ccl::comm_attr_t& attr) const
{
    LOG_TRACE("Create host communicator");
    ccl::communicator_interface_ptr impl =
            ccl::communicator_interface::create_communicator_impl(attr ? create_host_comm_attr() : attr);
    return ccl::communicator_t(new ccl::communicator(impl));
}

template<class stream_native_type, typename T>
CCL_API ccl::stream_t ccl::environment::create_stream(stream_native_type& s)
{
    return stream_t(new ccl::stream(stream_provider_dispatcher::create(s)));
}

#define STREAM_CREATOR_INSTANTIATION(type)                                                                                                           \
template ccl::stream_t CCL_API ccl::environment::create_stream(type& stream);

#ifdef CCL_ENABLE_SYCL
STREAM_CREATOR_INSTANTIATION(cl::sycl::queue)
#endif

CCL_API ccl::stream_t ccl::environment::create_stream() const
{
    return stream_t(new ccl::stream(stream_provider_dispatcher::create()));
}

CCL_API ccl::environment::~environment()
{
    if (env_ref_counter-- == 1)
    {
        auto result = ccl_finalize();
        if (result != ccl_status_success)
        {
            abort();
        }
    }
}

ccl::datatype CCL_API ccl::datatype_create(const ccl::datatype_attr* attr)
{
    ccl_datatype_t dtype;
    ccl_datatype_create(&dtype, attr);
    return static_cast<ccl::datatype>(dtype);
}

void CCL_API ccl::datatype_free(ccl::datatype dtype)
{
    ccl_datatype_free(dtype);
}

size_t CCL_API ccl::datatype_get_size(ccl::datatype dtype)
{
    size_t size;
    ccl_get_datatype_size(dtype, &size);
    return size;
}
/*
CCL_API ccl::stream::stream()
{
    this->stream_impl = std::make_shared<ccl_stream>(static_cast<ccl_stream_type_t>(ccl::stream_type::cpu),
                                                     nullptr / * native_stream * /);
}
*/
CCL_API ccl::stream::stream(std::shared_ptr<ccl_stream>&& impl) :
 stream_impl(std::move(impl))
{
}

CCL_API ccl::communicator::communicator(std::shared_ptr<communicator_interface> impl):
    pimpl(std::move(impl))
{
}

CCL_API ccl::communicator::~communicator()
{
}

CCL_API size_t ccl::communicator::rank() const
{
    return pimpl->rank();
}

CCL_API size_t ccl::communicator::size() const
{
    return pimpl->size();
}

CCL_API ccl::comm_attr_t ccl::communicator::get_host_attr() const
{
    return pimpl->get_host_attr();
}

#ifdef MULTI_GPU_SUPPORT
CCL_API ccl::device_topology_type ccl::communicator::get_topology_type() const
{
    return pimpl->get_topology_type();
}

CCL_API ccl::communicator::device_native_reference_t ccl::communicator::get_device()
{
    return pimpl->get_device();
}

CCL_API ccl::device_comm_attr_t ccl::communicator::get_device_attr() const
{
    return pimpl->get_device_attr();
}
#endif

CCL_API bool ccl::communicator::is_ready() const
{
    return pimpl->is_ready();
}


/* allgatherv */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::allgatherv(const void* send_buf,
                              size_t send_count,
                              void* recv_buf,
                              const size_t* recv_counts,
                              ccl::datatype dtype,
                              const ccl::coll_attr* attr,
                              const ccl::stream_t& stream)
{
    return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                             static_cast<ccl_datatype_t>(dtype),
                             attr,
                             (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type,
         typename T>
CCL_API ccl::communicator::coll_request_t
ccl::communicator::allgatherv(const buffer_type* send_buf,
                              size_t send_count,
                              buffer_type* recv_buf,
                              const size_t* recv_counts,
                              const ccl::coll_attr* attr,
                              const ccl::stream_t& stream)
{
    return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                            attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::allgatherv(const buffer_container_type& send_buf,
                              size_t send_count,
                              buffer_container_type& recv_buf,
                              const size_t* recv_counts,
                              const ccl::coll_attr* attr,
                              const ccl::stream_t& stream)
{
    return pimpl->allgatherv(send_buf, send_count, recv_buf, recv_counts,
                            attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

/* allreduce */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::allreduce(const void* send_buf,
                             void* recv_buf,
                             size_t count,
                             ccl::datatype dtype,
                             ccl::reduction reduction,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->allreduce(send_buf, recv_buf, count,
                            static_cast<ccl_datatype_t>(dtype),
                            reduction, attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::allreduce(const buffer_type* send_buf,
                             buffer_type* recv_buf,
                             size_t count,
                             ccl::reduction reduction,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->allreduce(send_buf, recv_buf, count, reduction, attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::allreduce(const buffer_container_type& send_buf,
                             buffer_container_type& recv_buf,
                             size_t count,
                             ccl::reduction reduction,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->allreduce(send_buf, recv_buf, count, reduction, attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

/* alltoall */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoall(const void* send_buf,
                            void* recv_buf,
                            size_t count,
                            ccl::datatype dtype,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           static_cast<ccl_datatype_t>(dtype),
                           attr,
                           (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type, typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoall(const buffer_type* send_buf,
                            buffer_type* recv_buf,
                            size_t count,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream)
{
    return pimpl->alltoall(send_buf, recv_buf, count,
                           attr,
                           (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type, typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoall(const buffer_container_type& send_buf,
                            buffer_container_type& recv_buf,
                            size_t count,
                            const ccl::coll_attr* attr,
                            const ccl::stream_t& stream)
{
    return pimpl->alltoall(send_buf, recv_buf, count, attr,
                           (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

/* alltoallv */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoallv(const void* send_buf,
                             const size_t* send_counts,
                             void* recv_buf,
                             const size_t* recv_counts,
                             ccl::datatype dtype,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            static_cast<ccl_datatype_t>(dtype),
                            attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type, typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoallv(const buffer_type* send_buf,
                             const size_t* send_counts,
                             buffer_type* recv_buf,
                             const size_t* recv_counts,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type, typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::alltoallv(const buffer_container_type& send_buf,
                             const size_t* send_counts,
                             buffer_container_type& recv_buf,
                             const size_t* recv_counts,
                             const ccl::coll_attr* attr,
                             const ccl::stream_t& stream)
{
    return pimpl->alltoallv(send_buf, send_counts, recv_buf, recv_counts,
                            attr,
                            (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

/* barrier */
void CCL_API
ccl::communicator::barrier(const ccl::stream_t& stream)
{
    pimpl->barrier((stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

/* bcast */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::bcast(void* buf,
                         size_t count,
                         ccl::datatype dtype,
                         size_t root,
                         const ccl::coll_attr* attr,
                         const ccl::stream_t& stream)
{
    return pimpl->bcast(buf, count,
                        static_cast<ccl_datatype_t>(dtype),
                        root, attr,
                        (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::bcast(buffer_type* buf,
                         size_t count,
                         size_t root,
                         const ccl::coll_attr* attr,
                         const ccl::stream_t& stream)

{
    return pimpl->bcast(buf, count, root, attr,
                        (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::bcast(buffer_container_type& buf,
                         size_t count,
                         size_t root,
                         const ccl::coll_attr* attr,
                         const ccl::stream_t& stream)
{
    return pimpl->bcast(buf, count, root, attr,
                        (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}


/* reduce */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::reduce(const void* send_buf,
                          void* recv_buf,
                          size_t count,
                          ccl::datatype dtype,
                          ccl::reduction reduction,
                          size_t root,
                          const ccl::coll_attr* attr,
                          const ccl::stream_t& stream)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         static_cast<ccl_datatype_t>(dtype),
                         reduction, root, attr,
                         (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::reduce(const buffer_type* send_buf,
                          buffer_type* recv_buf,
                          size_t count,
                          ccl::reduction reduction,
                          size_t root,
                          const ccl::coll_attr* attr,
                          const ccl::stream_t& stream)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         reduction, root, attr,
                         (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class buffer_container_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::reduce(const buffer_container_type& send_buf,
                          buffer_container_type& recv_buf,
                          size_t count,
                          ccl::reduction reduction,
                          size_t root,
                          const ccl::coll_attr* attr,
                          const ccl::stream_t& stream)
{
    return pimpl->reduce(send_buf, recv_buf, count,
                         reduction, root, attr,
                         (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}


/* sparse_allreduce */
ccl::communicator::coll_request_t CCL_API
ccl::communicator::sparse_allreduce(const void* send_ind_buf, size_t send_ind_count,
                                    const void* send_val_buf, size_t send_val_count,
                                    void** recv_ind_buf, size_t* recv_ind_count,
                                    void** recv_val_buf, size_t* recv_val_count,
                                    ccl::datatype index_dtype,
                                    ccl::datatype value_dtype,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    const ccl::stream_t& stream)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   static_cast<ccl_datatype_t>(index_dtype),
                                   static_cast<ccl_datatype_t>(value_dtype),
                                   reduction,
                                   attr,
                                   (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class index_buffer_type,
         class value_buffer_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::sparse_allreduce(const index_buffer_type* send_ind_buf, size_t send_ind_count,
                                    const value_buffer_type* send_val_buf, size_t send_val_count,
                                    index_buffer_type** recv_ind_buf, size_t* recv_ind_count,
                                    value_buffer_type** recv_val_buf, size_t* recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    const ccl::stream_t& stream)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   reduction,
                                   attr,
                                   (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

template<class index_buffer_container_type,
         class value_buffer_container_type,
         typename T>
ccl::communicator::coll_request_t CCL_API
ccl::communicator::sparse_allreduce(const index_buffer_container_type& send_ind_buf, size_t send_ind_count,
                                    const value_buffer_container_type& send_val_buf, size_t send_val_count,
                                    index_buffer_container_type** recv_ind_buf, size_t* recv_ind_count,
                                    value_buffer_container_type** recv_val_buf, size_t* recv_val_count,
                                    ccl::reduction reduction,
                                    const ccl::coll_attr* attr,
                                    const ccl::stream_t& stream)
{
    return pimpl->sparse_allreduce(send_ind_buf, send_ind_count,
                                   send_val_buf, send_val_count,
                                   recv_ind_buf, recv_ind_count,
                                   recv_val_buf, recv_val_count,
                                   reduction,
                                   attr,
                                   (stream) ? stream->stream_impl : ccl::get_empty_stream()->stream_impl);
}

#include "types_generator_defines.hpp"
#include "ccl_cpp_api_explicit_in.hpp"
