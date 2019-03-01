#include "out_of_order/tensor_comm.hpp"
#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/sched.hpp"
#include "postponed_sched.hpp"
#include "mlsl.hpp"

#include <thread>

mlsl_comm* out_of_order::tensor_to_comm_handler::get_comm_for_tensor(const std::string& tensor_name)
{
    std::lock_guard<std::mutex> lock(sync_guard);
    auto comm = tensor_to_comm_map.find(tensor_name);
    if (comm != tensor_to_comm_map.end())
    {
        MLSL_LOG(DEBUG, "Comm for tensor %s has been found", tensor_name.c_str());
        return comm->second.get();
    }
    MLSL_LOG(DEBUG, "No comm for tensor %s has been found", tensor_name.c_str());
    return nullptr;
}

void out_of_order::tensor_to_comm_handler::add_comm_for_tensor(const std::string& tensor_name,
                                                               std::shared_ptr<mlsl_comm> comm)
{
    std::lock_guard<std::mutex> lock(sync_guard);
    if (tensor_to_comm_map.find(tensor_name) != tensor_to_comm_map.end())
    {
        MLSL_LOG(ERROR, "tensor %s already exists", tensor_name.c_str());
        return;
    }
    tensor_to_comm_map.emplace(tensor_name, comm);
}
