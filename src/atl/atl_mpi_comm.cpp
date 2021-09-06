#ifdef CCL_ENABLE_MPI

#include "atl/atl_mpi_comm.h"
#include "exec/exec.hpp"

atl_mpi_comm::~atl_mpi_comm() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    tag.reset();
}

atl_mpi_comm::atl_mpi_comm() {
    if (!transport) {
        transport = std::shared_ptr<atl_mpi>(new atl_mpi());
    }

    init_transport();
}

atl_mpi_comm::atl_mpi_comm(std::shared_ptr<ikvs_wrapper> k) : atl_mpi_comm() {
    (void)k;
}

atl_mpi_comm::atl_mpi_comm(int total_rank_count,
                           const std::vector<int>& ranks,
                           std::shared_ptr<ikvs_wrapper> k)
        : atl_mpi_comm() {
    (void)total_rank_count;
    (void)ranks;
    (void)k;
}

void atl_mpi_comm::init_transport() {
    LOG_DEBUG("init ATL, requested ep_count ", attr.in.ep_count);
    static std::mutex memory_mutex;
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        if (!transport->is_inited()) {
            CCL_THROW_IF_NOT(
                transport->atl_init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                "failed to initialize ATL");
        }
    }
    eps = transport->atl_get_eps();

    threads_per_process = 1;
    ranks_per_process = 1;
    rank = transport.get()->get_rank();
    size = transport.get()->get_size();

    init_tag();

    executor_update();
}
#endif //CCL_ENABLE_MPI
