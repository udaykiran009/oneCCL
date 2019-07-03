#pragma once

#include "common/log/log.hpp"

#include <atomic>

class sync_object
{
public:
    explicit sync_object(size_t count) : initial_cnt(count), sync(count)
    {
        ICCL_ASSERT(initial_cnt > 0, "count must be greater than 0");
    }

    void visit()
    {
        auto value = sync.fetch_sub(1, std::memory_order_release);
        ICCL_ASSERT(value >= 0 && value <= initial_cnt, "invalid count ", value);
    }

    void reset()
    {
        sync.store(initial_cnt, std::memory_order_release);
    }

    size_t value() const
    {
        return sync.load(std::memory_order_acquire);
    }

private:
    size_t initial_cnt{};
    std::atomic_size_t sync{};
};
