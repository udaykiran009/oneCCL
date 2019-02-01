#pragma once

#include "sched/entry_types/entry.hpp"
#include "common/global/global.hpp"

class deregister_entry : public sched_entry
{
public:
    deregister_entry() = delete;
    deregister_entry(mlsl_sched* sched,
                     std::list<atl_mr_t*>& mr_list) :
        sched_entry(sched, true), mr_list(mr_list)
    {}

    void start_derived()
    {
        MLSL_LOG(DEBUG, "DEREGISTER entry sched %p, mr_count %zu", sched, mr_list.size());
        atl_status_t atl_status;
        std::list<atl_mr_t*>::iterator it;
        for (it = mr_list.begin(); it != mr_list.end(); it++)
        {
            MLSL_LOG(DEBUG, "deregister mr %p", *it);
            atl_status = atl_mr_dereg(global_data.executor->atl_desc, *it);
            if (unlikely(atl_status != atl_status_success))
            {
                status = mlsl_sched_entry_status_failed;
                MLSL_LOG(ERROR, "DEREGISTER entry failed. atl_status: %d", atl_status);
                break;
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
    char* dump_detail(char* dump_buf) const
    {
        auto bytes_written = sprintf(dump_buf, "sched %p, mr_count %zu\n",
                                     sched, mr_list.size());
        return dump_buf + bytes_written;
    }

private:
    std::list<atl_mr_t*>& mr_list;
};
