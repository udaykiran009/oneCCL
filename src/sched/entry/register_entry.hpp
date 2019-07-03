#pragma once

#include "common/global/global.hpp"
#include "exec/exec.hpp"
#include "sched/entry/entry.hpp"

class register_entry : public sched_entry
{
public:
    register_entry() = delete;
    register_entry(iccl_sched* sched,
                   size_t size,
                   void* ptr,
                   atl_mr_t** mr) :
        sched_entry(sched, true), size(size), ptr(ptr), mr(mr)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        LOG_DEBUG("REGISTER entry size ", size, ", ptr ", ptr);
        ICCL_THROW_IF_NOT(size > 0 && ptr && mr, "incorrect input, size ", size, ", ptr ", ptr, " mr ", mr);
        atl_status_t atl_status = atl_mr_reg(global_data.executor->atl_desc, ptr, size, mr);
        sched->memory.mr_list.emplace_back(*mr);
        if (unlikely(atl_status != atl_status_success))
        {
            ICCL_THROW("REGISTER entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = iccl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REGISTER";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        iccl_logger::format(str,
                            "sz ", size,
                            ", ptr ", ptr,
                            ", mr ", mr,
                            "\n");
    }

private:
    size_t size;
    void* ptr;
    atl_mr_t** mr;
};
