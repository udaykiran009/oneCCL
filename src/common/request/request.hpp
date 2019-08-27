#pragma once

#include <atomic>

#include "common/log/log.hpp"

class alignas(CACHELINE_SIZE) ccl_request
{
public:
    using dump_func = std::function<void(std::ostream &)>;
#ifdef ENABLE_DEBUG    
    void set_dump_callback(dump_func &&callback)
    {
        dump_callback = std::move(callback);
    }
#endif

    ~ccl_request()
    {
        auto counter = completion_counter.load(std::memory_order_acquire);
        LOG_DEBUG("delete req ", this, " with counter ", counter);
        if (counter != 0)
        {
            LOG_ERROR("unexpected completion_counter ", counter);
        }
    }

    bool complete()
    {
        int prev_counter = completion_counter.fetch_sub(1, std::memory_order_release);
        CCL_THROW_IF_NOT(prev_counter > 0, "unexpected prev_counter ", prev_counter, ", req ", this);
        LOG_DEBUG("req ", this, ", counter ", prev_counter - 1);
        return (prev_counter == 1);
    }

    bool is_completed() const
    {
        auto counter = completion_counter.load(std::memory_order_acquire);

#ifdef ENABLE_DEBUG
        if (counter != 0)
        {
            ++complete_checks_count;
            if (complete_checks_count >= CHECK_COUNT_BEFORE_DUMP)
            {
                complete_checks_count = 0;
                dump_callback(std::cout);
            }
        }
#endif
        LOG_TRACE("req: ", this, ", counter ", counter);

        return counter == 0;
    }

    void set_counter(int counter)
    {
        LOG_DEBUG("req: ", this, ", set count ", counter);
        int current_counter = completion_counter.load(std::memory_order_acquire);
        CCL_THROW_IF_NOT(current_counter == 0, "unexpected counter ", current_counter);
        completion_counter.store(counter, std::memory_order_release);
    }

    mutable bool urgent = false;
private:
    std::atomic_int completion_counter { 0 };

#ifdef ENABLE_DEBUG
    dump_func dump_callback;
    mutable size_t complete_checks_count = 0;
    static constexpr const size_t CHECK_COUNT_BEFORE_DUMP = 10000000;
#endif
};
