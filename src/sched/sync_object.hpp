#pragma once

#include "common/log/log.hpp"

#include <atomic>

class sync_object
{
public:
    explicit sync_object(size_t count) : initial_cnt(count), sync(count)
    {
        MLSL_ASSERT_FMT(initial_cnt > 0, "count must be greater than 0");
    }

    void visit()
    {
        MLSL_ASSERT_FMT(sync > 0, "already completed");
        --sync;
    }

    void reset()
    {
        sync = initial_cnt;
    }

    size_t value() const
    {
        return sync.load();
    }

private:
    size_t initial_cnt{};
    std::atomic_size_t sync{};
};
