#pragma once

#include "sched/sched.hpp"

#include <unordered_map>
#include <mutex>

namespace out_of_order
{

class postponed_sched_storage
{
public:
    postponed_sched_storage() = default;

    void postpone_for_tensor(const std::string& tensor_name, mlsl_sched* sched);

    void run_scheds_for_tensor(const std::string& tensor_name,
                                   mlsl_comm* tensor_comm,
                                   mlsl_executor* executor);

    mlsl_comm* get_sched_comm_by_tensor(const std::string& tensor_name);

    bool is_in_progress(const std::string& tensor_name)
    {
        std::lock_guard<std::mutex> lock{sync_guard};
        return user_scheds.count(tensor_name) != 0;
    }

private:
    using postponed_user_scheds_t = std::unordered_multimap<std::string, mlsl_sched*>;
    postponed_user_scheds_t user_scheds{};

    std::mutex sync_guard{};
};

}
