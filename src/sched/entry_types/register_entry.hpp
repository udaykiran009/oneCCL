#pragma once

#include "sched/entry_types/entry.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

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

    void start_derived()
    {
        MLSL_LOG(DEBUG, "REGISTER entry size %zu, ptr %p", size, ptr);
        MLSL_THROW_IF_NOT(size > 0 && ptr && mr, "incorrect input, size %zu ptr %p mr %p", size, ptr, mr);
        atl_status_t atl_status = atl_mr_reg(global_data.executor->atl_desc, ptr, size, mr);
        sched->memory.mr_list.emplace_back(*mr);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("REGISTER entry failed. atl_status: %d", atl_status);
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
