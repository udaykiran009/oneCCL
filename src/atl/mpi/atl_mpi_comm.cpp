#ifdef CCL_ENABLE_MPI

#include "atl/mpi/atl_mpi_comm.hpp"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple.h"
#include "atl/util/pm/pmi_resizable_rt/pmi_resizable_simple_internal.h"
#include "exec/exec.hpp"

std::atomic<size_t> atl_mpi_comm::comm_count{ 0 };
atl_mpi* atl_mpi_comm::transport{ nullptr };

atl_mpi_comm::~atl_mpi_comm() {
    static std::mutex memory_mutex;
    std::lock_guard<std::mutex> lock(memory_mutex);
    tag.reset();
    comm_count--;
    if (comm_count.load() == 0) {
        transport->finalize(rank);
        delete transport;
        transport = nullptr;
    }
}

atl_mpi_comm::atl_mpi_comm() {
    init_transport(true /* new communicator */);
}

atl_mpi_comm::atl_mpi_comm(std::shared_ptr<ikvs_wrapper> k) : atl_mpi_comm() {
    (void)k;
}

atl_mpi_comm::atl_mpi_comm(int comm_size,
                           const std::vector<int>& comm_ranks,
                           std::shared_ptr<ikvs_wrapper> k) {
    std::shared_ptr<internal_kvs> kvs;
    if ((kvs = std::dynamic_pointer_cast<internal_kvs>(k)) != nullptr) {
        pmi = std::shared_ptr<ipmi>(new pmi_resizable_simple_internal(comm_size, comm_ranks, kvs));
    }
    else {
        pmi = std::shared_ptr<ipmi>(new pmi_resizable_simple(comm_size, comm_ranks, k));
    }

    init_transport(true /* new communicator */, comm_size, &comm_ranks);
}

atl_mpi_comm::atl_mpi_comm(std::vector<atl_mpi_ep_t>& parent_eps,
                           int parent_rank,
                           int parent_size,
                           int color) {
    this->parent_rank = parent_rank;
    this->parent_size = parent_size;

    transport->comm_split(parent_eps, eps, color, parent_eps[0].coord->local_idx);
    transport->coord_update(eps[0].mpi_comm, coord);
    rank = coord.global_idx;
    size = coord.global_count;
    init_transport(false /* copy of communicator */);
    rank2rank_map.resize(size);
    MPI_Allgather(&parent_rank, 1, MPI_INT, rank2rank_map.data(), 1, MPI_INT, eps[0].mpi_comm);
}

void atl_mpi_comm::eps_update() {
    for (auto& ep : eps) {
        ep.coord = &coord;
    }
}

std::shared_ptr<atl_base_comm> atl_mpi_comm::comm_split(int color) {
    std::shared_ptr<atl_mpi_comm> comm =
        std::shared_ptr<atl_mpi_comm>(new atl_mpi_comm(eps, parent_rank, parent_size, color));

    return static_cast<std::shared_ptr<atl_mpi_comm>>(comm);
}

atl_status_t atl_mpi_comm::init_transport(bool is_new,
                                          int comm_size,
                                          const std::vector<int>* comm_ranks) {
    LOG_DEBUG("init ATL, requested ep_count ", attr.in.ep_count);
    if (is_new) {
        MPI_Comm global_comm = MPI_COMM_WORLD;
        static std::mutex memory_mutex;
        {
            if (pmi) {
                ATL_CHECK_STATUS(pmi->pmrt_init(), "pmi init failed");
                global_comm = MPI_COMM_NULL;
            }
            std::lock_guard<std::mutex> lock(memory_mutex);
            if (!transport) {
                transport = new atl_mpi();
            }
            if (!transport->is_inited()) {
                CCL_THROW_IF_NOT(
                    transport->init(nullptr, nullptr, &attr, nullptr, pmi) == ATL_STATUS_SUCCESS,
                    "failed to initialize ATL");
            }
            if (global_comm == MPI_COMM_NULL) {
                ATL_CHECK_STATUS(transport->comm_create(comm_size, *comm_ranks, pmi, &global_comm),
                                 "comm_create error");
            }
            int mpi_rank;
            MPI_Comm_rank(global_comm, &mpi_rank);
            if (mpi_rank == 0) {
                transport->printf_info();
                print_atl_attrs();
            }
        }

        transport->coord_update(global_comm, coord);
        transport->ep_init(eps, global_comm, coord.local_idx);
        parent_rank = rank = coord.global_idx;
        parent_size = size = coord.global_count;
        rank2rank_map.resize(size);

        for (int i = 0; i < size; i++) {
            rank2rank_map[i] = i;
        }
    }

    threads_per_process = 1;
    ranks_per_process = 1;

    eps_update();
    init_tag();

    comm_count++;

    executor_update();
    return ATL_STATUS_SUCCESS;
}
std::vector<int> atl_mpi_comm::get_rank2rank_map() {
    return rank2rank_map;
}
#endif //CCL_ENABLE_MPI
