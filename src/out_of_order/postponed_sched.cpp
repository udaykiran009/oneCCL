#include <exec/exec.hpp>
#include "postponed_sched.hpp"

void out_of_order::postponed_sched_storage::postpone_for_tensor(const std::string& tensor_name, mlsl_sched* sched)
{
    std::lock_guard<std::mutex> lock{sync_guard};
    MLSL_LOG(DEBUG, "Storage contains %zu entries for tensor %s", user_scheds.count(tensor_name), tensor_name.c_str());
    user_scheds.emplace(tensor_name, sched);
}

mlsl_comm* out_of_order::postponed_sched_storage::get_sched_comm_by_tensor(const std::string& tensor_name)
{
    std::lock_guard<std::mutex> lock{sync_guard};

    //return communicator for the first (and may be only) schedule.
    auto first_schedule = user_scheds.find(tensor_name);
    if(first_schedule != user_scheds.end())
    {
        return first_schedule->second->coll_param.comm;
    }
    return nullptr;
}

void out_of_order::postponed_sched_storage::run_scheds_for_tensor(const std::string& tensor_name,
                                                                  mlsl_comm* tensor_comm,
                                                                  mlsl_executor* executor)
{
    MLSL_THROW_IF_NOT(tensor_comm, "incorrect comm");

    std::lock_guard<std::mutex> lock{sync_guard};
    auto scheds = user_scheds.equal_range(tensor_name);

    MLSL_LOG(DEBUG, "Found %td scheds for tensor %s", std::distance(scheds.first, scheds.second),
             tensor_name.c_str());

    if (scheds.first != user_scheds.end() || scheds.second != user_scheds.end())
    {
        for (auto sched_it = scheds.first; sched_it != scheds.second; ++sched_it)
        {
            MLSL_LOG(INFO, "Running sched %p, type %s for tensor %s", sched_it->second,
                mlsl_coll_type_to_str(sched_it->second->coll_param.ctype),  sched_it->first.c_str());
            //user who started postponed collective has already had a request object
            mlsl_sched* schedule_obj = sched_it->second;
            //update communicator & run
            schedule_obj->coll_param.comm = tensor_comm;
            for (size_t part_idx = 0; part_idx < schedule_obj->partial_scheds.size(); ++part_idx)
            {
                schedule_obj->partial_scheds[part_idx]->coll_param.comm = tensor_comm;
            }

            schedule_obj->start(executor);
        }

        MLSL_LOG(DEBUG, "Erase postponed scheds for tensor %s if any", tensor_name.c_str());
        user_scheds.erase(scheds.first, scheds.second);
    }
}
