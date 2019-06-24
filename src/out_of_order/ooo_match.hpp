#pragma once

#include "exec/exec.hpp"
#include "common/comm/comm.hpp"
#include "common/log/log.hpp"

#include <memory>

struct ooo_runtime_info;

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
    const ooo_match& operator=(const ooo_match& other) = delete;

    ooo_match(mlsl_executor& exec,
              mlsl_comm_id_storage& comm_ids);

    /**
     * Searches for already created communicator for specific match_id
     * @param match_id user defined identifier
     * @return pointer to the communicator or null pointer of no communicator has been created for the provided match_id
     */
    mlsl_comm* get_comm(const std::string& match_id);

    /**
     * Moves the schedule to the postponed schedules storage
     * @param sched the schedule to be postponed
     */
    void postpone(mlsl_sched* sched);

    /**
     * Creates a communicator for specific match_id and runs all schedules postponed for this match_id.
     * Should be called after notification from the root rank. Communicator is created as a copy of one stored in
     * the postponed schedule
     * @param ctx internal runtime context that contains match_id
     */
    void create_comm_and_run_sched(ooo_runtime_info* ctx);

private:

    void bcast_match_id(const std::string& match_id);

    void update_comm_and_run(mlsl_sched* sched,
                             mlsl_comm* comm);

    mlsl_executor& executor;
    mlsl_comm_id_storage& comm_ids;

    //dedicated service communicator
    std::shared_ptr<mlsl_comm> service_comm;

    //collection of unresolved communicators
    using unresolved_comms_type = std::unordered_map<std::string, std::unique_ptr<comm_id>>;
    unresolved_comms_type unresolved_comms{};

    //collection of created communicators
    using match_id_to_comm_map_type = std::unordered_map<std::string, std::shared_ptr<mlsl_comm>>;
    match_id_to_comm_map_type match_id_to_comm_map;
    mlsl_spinlock match_id_to_comm_map_guard;

    void add_comm(std::string match_id,
                  std::shared_ptr<mlsl_comm> comm);

    //collection of postponed schedules
    using postponed_user_scheds_t = std::unordered_multimap<std::string, mlsl_sched*>;
    postponed_user_scheds_t postponed_user_scheds{};
    mlsl_spinlock postponed_user_scheds_guard{};

    void postpone_user_sched(mlsl_sched* sched);

    void run_postponed(const std::string& match_id,
                       mlsl_comm* comm);

    mlsl_comm* get_user_comm(const std::string& match_id);

    bool is_bcast_in_progress(const std::string& match_id);
};

}
