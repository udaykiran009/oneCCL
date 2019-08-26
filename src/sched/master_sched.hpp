#pragma once
#include "sched/sched_base.hpp"

class ccl_sched;
class ccl_master_sched : public ccl_sched_base, public ccl_request
{
public:
    static constexpr const char* class_name()
    {
        return "master_sched";
    }
    
    ccl_master_sched(ccl_coll_param& coll_param)
        : ccl_sched_base(coll_param),
          ccl_request(), 
          partial_scheds()
    {
#ifdef ENABLE_DEBUG    
    set_dump_callback([this](std::ostream &out)
                      {
                            dump(out);
                      });
#endif
    }

    ccl_master_sched(const ccl_master_sched &src) = delete;
    ~ccl_master_sched();

    
    void add_partial_sched(ccl_coll_param& param);
    void commit(ccl_parallelizer* parallelizer = nullptr);
    ccl_request* start(ccl_executor* exec,
                       bool reset_sched = true);
    
    /**
     * Reset completion counter of @b req
     * @return pointer to req that can be used to track completion
     */
    ccl_request* reset_request();
    /**
     * Synchronizes partial schedules on local barrier
     */
    void sync_partial_scheds();
    void dump(std::ostream &out) const;
    
    //TODO encapsulate it in private.
    std::vector<std::shared_ptr<ccl_sched>> partial_scheds;
private:
    void prepare_partial_scheds();
};
