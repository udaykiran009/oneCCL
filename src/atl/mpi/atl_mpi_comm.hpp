#pragma once

#ifdef CCL_ENABLE_MPI

#include <mpi.h>

#include "atl/atl_base_comm.hpp"
#include "atl/mpi/atl_mpi.hpp"

class atl_mpi_comm : public atl_base_comm {
public:
    ~atl_mpi_comm() = default;

    atl_mpi_comm();
    atl_mpi_comm(std::shared_ptr<ikvs_wrapper> k);
    atl_mpi_comm(int comm_size,
                 const std::vector<int>& local_ranks,
                 std::shared_ptr<ikvs_wrapper> k);

    atl_status_t finalize() override {
        transport->comms_free(eps);
        return ATL_STATUS_SUCCESS;
    }

    std::shared_ptr<atl_base_comm> comm_split(int color) override;

private:
    friend atl_comm_manager;
    atl_mpi_comm(atl_mpi_comm* parent, int color);
    void update_eps();
    atl_status_t init_transport(bool is_new,
                                int comm_size = 0,
                                const std::vector<int>& comm_ranks = {});
};

#endif //CCL_ENABLE_MPI
