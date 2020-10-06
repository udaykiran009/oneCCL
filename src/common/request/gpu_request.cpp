#include "common/request/request.hpp"
#include "common/request/gpu_request.hpp"
#include "sched/gpu_sched.hpp"

namespace ccl {
gpu_request_impl::gpu_request_impl(std::unique_ptr<ccl_gpu_sched>&& sched)
        : gpu_sched(std::move(sched)) {
    if (!gpu_sched) {
        completed = true;
    }
}

gpu_request_impl::~gpu_request_impl() {
    if (!completed) {
        LOG_ERROR("not completed gpu request is destroyed");
    }
}

void gpu_request_impl::wait() {
    if (!completed && gpu_sched) {
        do {
            gpu_sched->do_progress();
            completed = gpu_sched->wait(0);
        } while (!completed);
    }
}

bool gpu_request_impl::test() {
    if (!completed && gpu_sched) {
        completed = gpu_sched->wait(0);
        gpu_sched->do_progress();
    }
    return completed;
}

bool gpu_request_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& gpu_request_impl::get_native() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

gpu_shared_request_impl::gpu_shared_request_impl(std::shared_ptr<ccl_gpu_sched>&& sched)
        : gpu_sched(std::move(sched)) {
    if (!gpu_sched) {
        completed = true;
    }
}

gpu_shared_request_impl::~gpu_shared_request_impl() {
    if (!completed) {
        LOG_ERROR("not completed shared gpu request is destroyed");
    }
}

void gpu_shared_request_impl::wait() {
    if (!completed && gpu_sched) {
        do {
            gpu_sched->do_progress();
            completed = gpu_sched->wait(0);
        } while (!completed);
    }
}

bool gpu_shared_request_impl::test() {
    if (!completed && gpu_sched) {
        completed = gpu_sched->wait(0);
        gpu_sched->do_progress();
    }
    return completed;
}

bool gpu_shared_request_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& gpu_shared_request_impl::get_native() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

gpu_shared_process_request_impl::gpu_shared_process_request_impl(
    std::shared_ptr<ccl_gpu_sched>&& sched) {}

gpu_shared_process_request_impl::~gpu_shared_process_request_impl() {}

void gpu_shared_process_request_impl::wait() {}

bool gpu_shared_process_request_impl::test() {
    return false;
}

bool gpu_shared_process_request_impl::cancel() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

event::native_t& gpu_shared_process_request_impl::get_native() {
    throw ccl::exception(std::string(__FUNCTION__) + " - is not implemented");
}

} // namespace ccl
