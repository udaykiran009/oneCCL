#include "common/global/global.hpp"
#include "common/utils/yield.hpp"
#include "exec/thread/base_thread.hpp"

ccl::status ccl_base_thread::start(int affinity) {
    LOG_DEBUG(name(), " ", idx);

    start_affinity = affinity;

    /* start thread with initial affinity */
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    cpu_set_t cpuset;
    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(affinity, sizeof(cpu_set_t), &cpuset);
    pthread_attr_setaffinity_np(&attr, sizeof(cpu_set_t), &cpuset);

    int err = pthread_create(&thread, &attr, progress_function, get_this());
    if (err) {
        LOG_ERROR(
            "error while creating ", name(), " thread #", idx, " pthread_create returns ", err);
        return ccl::status::runtime_error;
    }

    while (!started.load(std::memory_order_relaxed)) {
        ccl_yield(ccl::global_data::env().yield_type);
    }

    return ccl::status::success;
}

ccl::status ccl_base_thread::stop() {
    LOG_DEBUG(name(), " # ", idx);

    void* exit_code;
    int err;

    should_stop = true;

    while (started.load(std::memory_order_relaxed)) {
        ccl_yield(ccl::global_data::env().yield_type);
    }

    err = pthread_join(thread, &exit_code);
    if (err) {
        LOG_INFO("error while joining progress thread # ", idx, " , pthread_join returns ", err);
    }
    else {
        LOG_DEBUG("progress thread # ",
                  idx,
                  ", exited with code (",
                  (uintptr_t)exit_code,
                  (exit_code == PTHREAD_CANCELED) ? "PTHREAD_CANCELED" : "?",
                  ")");
    }

    return ccl::status::success;
}

ccl::status ccl_base_thread::set_affinity(int affinity) {
    LOG_DEBUG(name(), " # ", idx, ", affinity ", affinity);

    int pthread_err;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);
    __CPU_SET_S(affinity, sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_setaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0) {
        LOG_ERROR("pthread_setaffinity_np failed, err ", pthread_err);
        return ccl::status::runtime_error;
    }

    if (get_affinity() != affinity) {
        LOG_ERROR(name(), " ", idx, " is not pinned ", affinity);
        return ccl::status::runtime_error;
    }

    return ccl::status::success;
}

int ccl_base_thread::get_affinity() {
    int pthread_err;
    int result = CCL_UNDEFINED_CPU_ID;
    cpu_set_t cpuset;

    __CPU_ZERO_S(sizeof(cpu_set_t), &cpuset);

    if ((pthread_err = pthread_getaffinity_np(thread, sizeof(cpu_set_t), &cpuset)) != 0) {
        LOG_ERROR("pthread_getaffinity_np failed, err ", pthread_err);
    }

    for (int idx = 0; idx < CPU_SETSIZE; idx++) {
        if (__CPU_ISSET_S(idx, sizeof(cpu_set_t), &cpuset)) {
            if (result == CCL_UNDEFINED_CPU_ID) {
                result = idx;
            }
            else {
                CCL_THROW("multiple affinity cores, previous ", result, ", new ", idx);
            }
        }
    }

    CCL_THROW_IF_NOT(result != CCL_UNDEFINED_CPU_ID, "can't retrieve affinity");

    return result;
}
