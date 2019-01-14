#pragma once

#include "common/comm/comm.hpp"
#include "common/log/log.hpp"
#include "atl/atl.h"

#include <string>
#include <memory>
#include <unordered_map>
#include <set>
#include <mutex>

struct mlsl_sched_entry;
struct mlsl_sched;

namespace out_of_order
{

class tensor_to_comm_handler
{
public:
    tensor_to_comm_handler() = default;
    mlsl_comm* get_comm_for_tensor(const std::string& tensor_name);
    void add_comm_for_tensor(const std::string& tensor_name, mlsl_comm* comm);

private:
    using tensor_to_comm_map_type = std::unordered_map<std::string, std::shared_ptr<mlsl_comm>>;
    tensor_to_comm_map_type tensor_to_comm_map;
    tensor_to_comm_map_type unresolved_tensor_comms;
    std::mutex sync_guard;
};

}
