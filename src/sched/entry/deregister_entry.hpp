#pragma once

#include "common/global/global.hpp"
#include "sched/entry/entry.hpp"

class deregister_entry : public sched_entry
{
public:
    deregister_entry() = delete;
    deregister_entry(mlsl_sched* sched,
                     std::list<atl_mr_t*>& mr_list) :
        sched_entry(sched, true), mr_list(mr_list)
    {
        LOG_DEBUG("creating ", name(), " entry");
    }

    void start_derived()
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
                MLSL_THROW("DEREGISTER entry failed. atl_status: ", atl_status_to_str(atl_status));
            }
        }
        mr_list.clear();
        status = mlsl_sched_entry_status_complete;
    }

    const char* name() const
    {
        return "DEREGISTER";
    }

protected:
    void dump_detail(std::stringstream& str) const
    {
        mlsl_logger::format(str,
                            "sched ", sched,
                            ", mr_count ", sched, mr_list.size(),
                            "\n");

    }

private:
    std::list<atl_mr_t*>& mr_list;
};
