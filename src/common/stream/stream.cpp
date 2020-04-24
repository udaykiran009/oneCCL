#include "common/log/log.hpp"
#include "common/stream/stream.hpp"
#include "common/stream/stream_provider_dispatcher_impl.hpp"

#ifdef CCL_ENABLE_SYCL
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(cl::sycl::queue& native_stream);
#endif

#ifdef MULTI_GPU_SUPPORT
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(ze_command_queue_handle_t& native_stream);
#else
    template std::unique_ptr<ccl_stream> stream_provider_dispatcher::create(void*& native_stream);
#endif
