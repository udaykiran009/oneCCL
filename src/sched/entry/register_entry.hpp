#pragma once

#include "common/global/global.hpp"
#include "exec/exec.hpp"
#include "sched/entry/entry.hpp"

class register_entry : public sched_entry
{
public:
    static constexpr const char* class_name() noexcept
    {
        return "REGISTER";
    }

    register_entry() = delete;
    register_entry(ccl_sched* sched,
                   size_t size,
                   const ccl_buffer ptr,
                   atl_mr_t** mr) :
        sched_entry(sched, true), size(size), ptr(ptr), mr(mr)
    {
    }

    void start() override
    {
        LOG_DEBUG("REGISTER entry size ", size, ", ptr ", ptr);
        CCL_THROW_IF_NOT(size > 0 && ptr && mr, "incorrect input, size ", size, ", ptr ", ptr, " mr ", mr);
        atl_status_t atl_status = atl_mr_reg(global_data.executor->atl_desc, ptr.get_ptr(size), size, mr);
        sched->memory.mr_list.emplace_back(*mr);
        if (unlikely(atl_status != atl_status_success))
        {
            CCL_THROW("REGISTER entry failed. atl_status: ", atl_status_to_str(atl_status));
        }
        else
            status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "sz ", size,
                           ", ptr ", ptr,
                           ", mr ", mr,
                           "\n");
    }

private:
    size_t size;
    ccl_buffer ptr;
    atl_mr_t** mr;
};
