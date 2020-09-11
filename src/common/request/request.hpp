#pragma once

#include <atomic>
#include <functional>

#include "common/utils/utils.hpp"

class alignas(CACHELINE_SIZE) ccl_request {
public:
    using dump_func = std::function<void(std::ostream &)>;
#ifdef ENABLE_DEBUG
    void set_dump_callback(dump_func &&callback);
#endif

    virtual ~ccl_request();

    bool complete();

    bool is_completed() const;

    void set_counter(int counter);

    void increase_counter(int increment);

    mutable bool urgent = false;

private:
    std::atomic_int completion_counter{ 0 };

#ifdef ENABLE_DEBUG
    dump_func dump_callback;
    mutable size_t complete_checks_count = 0;
    static constexpr const size_t CHECK_COUNT_BEFORE_DUMP = 40000000;
#endif
};
