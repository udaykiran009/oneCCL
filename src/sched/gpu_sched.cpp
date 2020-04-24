#include <unistd.h>

#include "sched/gpu_sched.hpp"


gpu_request_impl::gpu_request_impl( std::unique_ptr<ccl_gpu_sched>&& sched) :
    gpu_sched(std::move(sched))
{
    if (!gpu_sched)
    {
        completed = true;
    }
}

gpu_request_impl::~gpu_request_impl()
{
    if (!completed)
    {
        LOG_ERROR("not completed gpu request is destroyed");
    }
}

void gpu_request_impl::wait()
{
    if (!completed && gpu_sched)
    {
        do
        {
            gpu_sched->do_progress();
            completed = gpu_sched->wait(0);
        }
        while(!completed);
    }
}

bool gpu_request_impl::test()
{
    if (!completed && gpu_sched)
    {
        completed = gpu_sched->wait(0);
        gpu_sched->do_progress();
    }
    return completed;
}


gpu_shared_request_impl::gpu_shared_request_impl( std::shared_ptr<ccl_gpu_sched>&& sched) :
    gpu_sched(std::move(sched))
{
    if (!gpu_sched)
    {
        completed = true;
    }
}

gpu_shared_request_impl::~gpu_shared_request_impl()
{
    if (!completed)
    {
        LOG_ERROR("not completed shared gpu request is destroyed");
    }
}

void gpu_shared_request_impl::wait()
{
    if (!completed && gpu_sched)
    {
        do
        {
            gpu_sched->do_progress();
            completed = gpu_sched->wait(0);
        }
        while(!completed);
    }
}

bool gpu_shared_request_impl::test()
{
    if (!completed && gpu_sched)
    {
        completed = gpu_sched->wait(0);
        gpu_sched->do_progress();
    }
    return completed;
}



gpu_shared_process_request_impl::gpu_shared_process_request_impl( std::shared_ptr<ccl_gpu_sched>&& sched)
{
}

gpu_shared_process_request_impl::~gpu_shared_process_request_impl()
{
}

void gpu_shared_process_request_impl::wait()
{
}

bool gpu_shared_process_request_impl::test()
{
    return false;
}

ccl_gpu_sched::ccl_gpu_sched(native::specific_indexed_device_storage& devices,
              size_t group_size,
              const ccl_coll_param& coll_param/* = ccl_coll_param()*/)
 : ccl_sched(coll_param, nullptr),
   ccl_request(),
   expected_group_size(group_size),
   group_comm_devices(devices)
{
    //preallocation
    entry_fences.reserve(expected_group_size);
    //WHY deque??? entries.reserve(expected_group_size);
}

void ccl_gpu_sched::complete()
{
}

ze_fence_handle_t* entry_request = nullptr;
void ccl_gpu_sched::set_fence(ze_fence_handle_t entry_fence)//TODO temporary
{
    assert(entry_fence);
    entry_fences.push_back(entry_fence);
}

bool ccl_gpu_sched::wait(size_t nanosec)
{
    bool ready = true;
    for(auto fence : entry_fences)
    {
        ze_result_t ret = zeFenceHostSynchronize(fence, nanosec);
        if(ret != ZE_RESULT_SUCCESS)
        {
            if(ret == ZE_RESULT_NOT_READY or ret == ZE_RESULT_ERROR_INVALID_ARGUMENT)
            {
                ready = false;
                break;
            }
            throw std::runtime_error(std::string("zeFenceHostSynchronize failed: ") + native::to_string(ret));
        }
    }
    return ready;
}
