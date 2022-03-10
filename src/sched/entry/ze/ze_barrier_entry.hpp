#pragma once

#include "sched/entry/factory/entry_factory.hpp"

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

    void finalize() override;

protected:
    void dump_detail(std::stringstream& str) const override;

private:
    const ccl_comm* comm;
    const int rank;
    const int comm_size;
    size_t last_completed_event_idx{};
    size_t wait_event_idx{};

    ze_event_pool_handle_t local_pool{};
    ze_event_handle_t signal_event{};
    std::vector<std::pair<int, ze_event_handle_t>> wait_events{};
};
