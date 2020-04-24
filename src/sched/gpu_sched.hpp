#pragma once
#include "sched/queue/queue.hpp"
#include "sched/sched.hpp"
#include "native_device_api/export_api.hpp"
#include "common/comm/l0/gpu_device_types.hpp"


class ccl_gpu_sched;

class gpu_request_impl final : public ccl::request
{
public:
    explicit gpu_request_impl( std::unique_ptr<ccl_gpu_sched>&& sched);
    ~gpu_request_impl();

    void wait() override;
    bool test() override;

private:
    std::unique_ptr<ccl_gpu_sched> gpu_sched;
    bool completed = false;
};


class gpu_shared_request_impl final : public ccl::request
{
public:
    explicit gpu_shared_request_impl( std::shared_ptr<ccl_gpu_sched>&& sched);
    ~gpu_shared_request_impl();

    void wait() override;
    bool test() override;

private:
    std::shared_ptr<ccl_gpu_sched> gpu_sched;
    bool completed = false;
};


class gpu_shared_process_request_impl final : public ccl::request
{
public:
    explicit gpu_shared_process_request_impl( std::shared_ptr<ccl_gpu_sched>&& sched);
    ~gpu_shared_process_request_impl();

    void wait() override;
    bool test() override;

private:
    std::shared_ptr<ccl_gpu_sched> gpu_sched;
    bool completed = false;
};



class alignas(CACHELINE_SIZE) ccl_gpu_sched : public ccl_sched, public ccl_request
{
public:
    static constexpr const char* class_name()
    {
        return "gpu_worker_sched";
    }

    ccl_gpu_sched(native::specific_indexed_device_storage& devices,
                  size_t expected_group_size,
                  const ccl_coll_param& coll_param = ccl_coll_param());

    ccl_gpu_sched(const ccl_sched& other) = delete;
    ccl_gpu_sched& operator= (const ccl_gpu_sched& other) = delete;

    ~ccl_gpu_sched() = default;


    void complete() override;
    bool wait(size_t nanosec);
    //TODO temporary
    void set_fence(ze_fence_handle_t entry_fence);


    template<class device_t>
    native::indexed_device_container<device_t>& get_devices()
    {
        return std::get<device_t::type_idx()>(group_comm_devices);
    }

    size_t get_group_size() const
    {
        return expected_group_size;
    }
private:
    size_t expected_group_size;
    std::vector<ze_fence_handle_t> entry_fences;
    native::specific_indexed_device_storage& group_comm_devices;
};
