#pragma once
#include <memory>

#include "ccl.hpp"

class ccl_gpu_sched;

namespace ccl {

class gpu_request_impl final : public ccl::request {
public:
    explicit gpu_request_impl(std::unique_ptr<ccl_gpu_sched>&& sched);
    ~gpu_request_impl();

    void wait() override;
    bool test() override;

private:
    std::unique_ptr<ccl_gpu_sched> gpu_sched;
    bool completed = false;
};

class gpu_shared_request_impl final : public ccl::request {
public:
    explicit gpu_shared_request_impl(std::shared_ptr<ccl_gpu_sched>&& sched);
    ~gpu_shared_request_impl();

    void wait() override;
    bool test() override;

private:
    std::shared_ptr<ccl_gpu_sched> gpu_sched;
    bool completed = false;
};

class gpu_shared_process_request_impl final : public ccl::request {
public:
    explicit gpu_shared_process_request_impl(std::shared_ptr<ccl_gpu_sched>&& sched);
    ~gpu_shared_process_request_impl();

    void wait() override;
    bool test() override;

private:
    std::shared_ptr<ccl_gpu_sched> gpu_sched;
};
} // namespace ccl
