#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"

class deregister_entry : public sched_entry
{
public:
    static constexpr const char* entry_class_name() noexcept
    {
        return "DEREGISTER";
    }

    deregister_entry() = delete;
    deregister_entry(ccl_sched* sched,
                     std::list<atl_mr_t*>& mr_list) :
        sched_entry(sched, true), mr_list(mr_list)
    {
    }

    void start_derived() override
    {
        LOG_DEBUG("DEREGISTER entry sched ", sched, " mr_count ", mr_list.size());
        atl_status_t atl_status;
        std::list<atl_mr_t*>::iterator it;
        for (it = mr_list.begin(); it != mr_list.end(); it++)
        {
            LOG_DEBUG("deregister mr ", *it);
            atl_status = atl_mr_dereg(global_data.executor->atl_desc, *it);
            if (unlikely(atl_status != atl_status_success))
            {
                CCL_THROW("DEREGISTER entry failed. atl_status: ", atl_status_to_str(atl_status));
            }
        }
        mr_list.clear();
        status = ccl_sched_entry_status_complete;
    }

    const char* name() const override
    {
        return entry_class_name();
    }

protected:
    void dump_detail(std::stringstream& str) const override
    {
        ccl_logger::format(str,
                           "sched ", sched,
                           ", mr_count ", sched, mr_list.size(),
                           "\n");

    }

private:
    std::list<atl_mr_t*>& mr_list;
};
