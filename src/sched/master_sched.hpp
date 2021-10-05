#pragma once

#include "sched/sched_base.hpp"

class ccl_sched;
class ccl_sched_key;

class ccl_master_sched : public ccl_sched_base, public ccl_request {
public:
    static constexpr const char* class_name() {
        return "master_sched";
    }

    ccl_master_sched(const ccl_sched_create_param& param);

    ccl_master_sched(const ccl_master_sched& src) = delete;

    ~ccl_master_sched() override;

    void add_partial_sched(const ccl_coll_param& param);
    void commit(ccl_parallelizer* parallelizer = nullptr);
    ccl_request* start(ccl_executor* exec, bool reset_sched = true);

    /**
     * Reset completion counter of @b req
     * @return pointer to req that can be used to track completion
     */
    ccl_request* reset_request();
    /**
     * Synchronizes partial schedules on local barrier
     */
    void sync_partial_scheds();
    void dump(std::ostream& out) const;

    // TODO encapsulate it in private.
    std::vector<std::shared_ptr<ccl_sched>> partial_scheds;

    // TODO: wrap into smart-pointer
    using ccl_master_sched_ptr = ccl_master_sched*;
    static ccl_master_sched_ptr create(const ccl_coll_param& param, const ccl_coll_attr& attr);

#ifdef CCL_ENABLE_SYCL
    bool print_kernel_timer() const;
    void reset_kernel_timer();

    ccl::kernel_timer& get_kernel_timer() {
        return kernel_timer;
    }
#endif // CCL_ENABLE_SYCL

private:
    void reset_state();
    void prepare_partial_scheds();

#ifdef CCL_ENABLE_SYCL
    ccl::kernel_timer kernel_timer;
#endif // CCL_ENABLE_SYCL
};
