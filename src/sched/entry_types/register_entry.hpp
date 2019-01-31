#pragma once

#include "sched/entry_types/entry.hpp"
#include "sched/sched_queue.hpp"
#include "common/global/global.hpp"

class register_entry : public sched_entry
{
public:
    register_entry() = delete;
    register_entry(mlsl_sched* sched,
                   size_t size,
                   void* ptr,
                   atl_mr_t** mr) :
        sched_entry(sched, true), size(size), ptr(ptr), mr(mr)
    {}

    ~register_entry()
    {
        if (mr && *mr)
        {
            MLSL_LOG(DEBUG, "DEREGISTER size %zu, ptr %p", size, ptr);
            atl_status_t atl_status = atl_mr_dereg(global_data.executor->atl_desc, *mr);
            MLSL_ASSERTP(atl_status == atl_status_success);
        }
    }

    void start_derived()
    {
        MLSL_LOG(DEBUG, "REGISTER entry size %zu, ptr %p", size, ptr);
        MLSL_ASSERTP(size > 0);
        MLSL_ASSERTP(ptr);
        MLSL_ASSERTP(mr);
        atl_status_t atl_status = atl_mr_reg(global_data.executor->atl_desc, ptr, size, mr);
        if (unlikely(atl_status != atl_status_success))
        {
            status = mlsl_sched_entry_status_failed;
            MLSL_LOG(ERROR, "REGISTER entry failed. atl_status: %d", atl_status);
        }
        else
            status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REGISTER";
    }

protected:
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "sz %zu, ptr %p, mr %p\n",
                                     size, ptr, mr);
        return dump_buf + bytes_written;
    }

private:
    size_t size;
    void* ptr;
    atl_mr_t** mr;
};
