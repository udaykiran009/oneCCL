#pragma once

#include "exec/exec.hpp"
#include "common/comm/comm.hpp"
#include "common/log/log.hpp"
#include "out_of_order/postponed_sched.hpp"
#include "out_of_order/tensor_comm.hpp"

#include <memory>

namespace out_of_order
{

/**
 * Provides common interface to the out of order subsystem, encapsulates storage of postponed schedules
 * and storage of already created communicators
 */
class ooo_match
{
public:
    ooo_match() = delete;
    ooo_match(const ooo_match& other) = delete;
    const ooo_match& operator= (const ooo_match& other) = delete;

    ooo_match(mlsl_executor& exec, comm_id_storage& comm_ids);

    /**
     * Searches for already created communicator for specific tensor name
     * @param tensor_name user defined tensor name
     * @return pointer to the communicator or null pointer of no communicator has been created for the provided tensor
     */
    mlsl_comm* get_comm_for_tensor(const std::string& tensor_name)
    {
        return m_tensor_comm_storage.get_comm_for_tensor(tensor_name);
    }

    /**
     * Checks if there is an active process of the communicator broadcasting for specific tensor name
     * @param tensor_name user defined tensor name
     * @return true if there is and active broadcast process, false otherwise
     */
    bool is_bcast_in_progress(const std::string& tensor_name)
    {
        return m_postponed_schedules.is_in_progress(tensor_name);
    }

    /**
     * Moves the schedule to the postponed schedules storage
     * @param tensor_name tensor name which the schedule depends on
     * @param sched the schedule to be postponed
     */
    void postpone_for_tensor(const std::string& tensor_name, mlsl_sched* sched);

    /**
     * Creates a communicator for specific tensor name and runs all schedules postponed for this tensor name.
     * Should be called after notification from the root rank. Communicator is created as a copy of one stored in
     * the postponed schedule
     * @param tensor_name user defined tensor name
     */
    void create_comm_and_run_sched(const std::string& tensor_name);

    /**
     * Builds a schedule to broadcast provided tensor name. Only root rank can pass a valid tensor name
     * @param tensor_name tensor name to be broadcasted
     * @return a built schedule
     */
    mlsl_sched* build_bcast_sched(const char* tensor_name = nullptr);

private:

    mlsl_executor& m_executor;
    comm_id_storage& m_comm_ids;
    out_of_order::tensor_to_comm_handler m_tensor_comm_storage {};
    out_of_order::postponed_sched_storage m_postponed_schedules {};
    std::shared_ptr<mlsl_comm> m_service_comm;
    std::unordered_map<std::string, std::unique_ptr<comm_id>> m_unresolved_comms {};
};

}
