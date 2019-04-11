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
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
    {
        LOG_DEBUG("REGISTER entry size ", size, ", ptr ", ptr);
        MLSL_THROW_IF_NOT(size > 0 && ptr && mr, "incorrect input, size ", size, ", ptr ", ptr, " mr ", mr);
        atl_status_t atl_status = atl_mr_reg(global_data.executor->atl_desc, ptr, size, mr);
        sched->memory.mr_list.emplace_back(*mr);
        if (unlikely(atl_status != atl_status_success))
        {
            MLSL_THROW("REGISTER entry failed. atl_status: ", atl_status);
        }
        else
            status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "REGISTER";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
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
