#pragma once

#include "sched/entry/factory/entry_factory.hpp"

#include <ze_api.h>

class ze_barrier_entry : public sched_entry {
public:
    static constexpr const char* class_name() noexcept {
        return "ZE_BARRIER";
    }

    const char* name() const noexcept override {
        return class_name();
    }

    ze_barrier_entry() = delete;
    explicit ze_barrier_entry(ccl_sched* sched,
                              ccl_comm* comm,
                              ze_event_pool_handle_t& local_pool,
                              size_t event_idx);
    ~ze_barrier_entry();

    void start() override;
    void update() override;

    void finalize();

protected:
    void dump_detail(std::stringstream& str) const override {
        ccl_logger::format(str,
                           "rank ",
                           rank,
                           ", comm_size ",
                           comm_size,
                           ", comm_id ",
                           sched->get_comm_id(),
                           "wait_events: ",
                           wait_events.size(),
                           "\n");
    }

private:
    ccl_comm* comm;
    const int rank;
    const int comm_size;
    size_t last_completed_event_idx{};
    size_t event_idx{};

    ze_event_pool_handle_t local_pool{};
    ze_event_handle_t signal_event{};
    std::vector<std::pair<int, ze_event_handle_t>> wait_events{};
};
