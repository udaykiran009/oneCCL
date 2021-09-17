#ifdef CCL_ENABLE_MPI

#include "atl/atl_mpi_comm.hpp"
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

atl_mpi_comm::atl_mpi_comm(std::shared_ptr<atl_mpi> transport,
                           std::vector<atl_mpi_ep_t>& parent_eps,
                           int parent_rank,
                           int parent_size,
                           int color)
        : transport(transport),
          parent_rank(parent_rank),
          parent_size(parent_size) {
    threads_per_process = 1;
    ranks_per_process = 1;

    transport->comm_split(parent_eps, eps, color);
    transport->coord_update(eps[0].mpi_comm, coord);
    rank = coord.global_idx;
    size = coord.global_count;
    eps_update();
    rank2rank_map.resize(size);
    MPI_Allgather(&parent_rank, 1, MPI_INT, rank2rank_map.data(), 1, MPI_INT, eps[0].mpi_comm);
    init_tag();
}

void atl_mpi_comm::eps_update() {
    for (auto& ep : eps) {
        ep.coord = &coord;
    }
}

std::shared_ptr<atl_base_comm> atl_mpi_comm::comm_split(size_t color) {
    std::shared_ptr<atl_mpi_comm> comm = std::shared_ptr<atl_mpi_comm>(
        new atl_mpi_comm(transport, eps, parent_rank, parent_size, color));

    return static_cast<std::shared_ptr<atl_mpi_comm>>(comm);
}

void atl_mpi_comm::init_transport() {
    LOG_DEBUG("init ATL, requested ep_count ", attr.in.ep_count);
    static std::mutex memory_mutex;
    {
        std::lock_guard<std::mutex> lock(memory_mutex);
        if (!transport->is_inited()) {
            CCL_THROW_IF_NOT(
                transport->init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                "failed to initialize ATL");
        }
    }

    threads_per_process = 1;
    ranks_per_process = 1;
    transport->ep_init(eps);
    transport->coord_update(MPI_COMM_WORLD, coord);
    parent_rank = rank = coord.global_idx;
    parent_size = size = coord.global_count;
    rank2rank_map.resize(size);
    for (int i = 0; i < size; i++) {
        rank2rank_map[i] = i;
    }
    eps_update();

    init_tag();

    executor_update();
}
std::vector<int> atl_mpi_comm::get_rank2rank_map() {
    return rank2rank_map;
}
#endif //CCL_ENABLE_MPI
