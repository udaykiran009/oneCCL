#ifdef CCL_ENABLE_MPI
#include "atl/mpi/atl_mpi.hpp"
#include "atl_mpi_comm.h"
#endif // CCL_ENABLE_MPI

#include "atl/atl_ofi_comm.h"
#include "atl/atl_base_comm.h"
#include "atl/ofi/atl_ofi.hpp"
#include "common/global/global.hpp"
#include "exec/exec.hpp"

atl_attr_t atl_base_comm::attr = {
    /* in */
    {
        0, /* enable_shm */
        0, /* enable_rma */
        0, /* enable_hmem */
        0, /* enable_sync_coll */
        0, /* enable_extra_ep */
        1, /* ep_count */
        ATL_MNIC_NONE, /* mnic_type */
        "", /* mnic_name */
        1 /* mnic_count */
    },

    /* out */
    {
        0, /* enable_shm */
        0, /* enable_rma */
        0, /* enable_hmem */
        ATL_MNIC_NONE, /* mnic_type */
        0, /* mnic_count */
        0, /* tag_bits */
        0, /* max_tag */
        0, /* max_order_waw_size */
    }
};

ccl_executor* atl_base_comm::executor = nullptr;

void atl_base_comm::init_tag() {
    tag = std::unique_ptr<ccl_atl_tag>(new ccl_atl_tag(attr.out.tag_bits, attr.out.max_tag));

    if (rank == 0) {
        tag->print();
        LOG_INFO("atl-in-attrs:");
        LOG_INFO("  enable_shm: ", attr.in.enable_shm);
        LOG_INFO("  enable_rma: ", attr.in.enable_rma);
        LOG_INFO("  enable_hmem: ", attr.in.enable_hmem);
        LOG_INFO("  enable_sync_coll: ", attr.in.enable_sync_coll);
        LOG_INFO("  enable_extra_ep: ", attr.in.enable_extra_ep);
        LOG_INFO("  ep_count: ", attr.in.ep_count);
        LOG_INFO("  mnic_type: ", attr.in.mnic_type);
        LOG_INFO("  mnic_count: ", attr.in.mnic_count);

        LOG_INFO("atl-out-attrs:");
        LOG_INFO("  enable_shm: ", attr.out.enable_shm);
        LOG_INFO("  enable_rma: ", attr.out.enable_rma);
        LOG_INFO("  enable_hmem: ", attr.out.enable_hmem);
        LOG_INFO("  mnic_type: ", attr.out.mnic_type);
        LOG_INFO("  mnic_count: ", attr.out.mnic_count);
        LOG_INFO("  tag_bits: ", attr.out.tag_bits);
        LOG_INFO("  max_tag: ", attr.out.max_tag);
        LOG_INFO("  max_order_waw_size: ", attr.out.max_order_waw_size);
    }
}

void atl_base_comm::executor_update() {
    if (!executor->are_workers_started()) {
        atl_proc_coord_t* coord = get_proc_coord();
        if (rank < coord->local_count)
            LOG_INFO("start workers for local process [",
                     coord->local_idx,
                     ":",
                     coord->local_count,
                     "]");
        executor->start_workers(coord->local_idx, coord->local_count);
    }
}

std::shared_ptr<atl_base_comm> atl_comm_manager::create_comm() {
    std::shared_ptr<atl_base_comm> atl_comm;

    auto transport_type = ccl::global_data::env().atl_transport;

    switch (transport_type) {
        case ccl_atl_ofi: atl_comm = std::shared_ptr<atl_base_comm>(new atl_ofi_comm()); break;
#ifdef CCL_ENABLE_MPI
        case ccl_atl_mpi: atl_comm = std::shared_ptr<atl_base_comm>(new atl_mpi_comm()); break;
#endif // CCL_ENABLE_MPI
        default: LOG_ERROR("Unsupported yet"); break;
    }
    return atl_comm;
}

std::shared_ptr<atl_base_comm> atl_comm_manager::create_comm(std::shared_ptr<ikvs_wrapper> k) {
    std::shared_ptr<atl_base_comm> atl_comm;

    auto transport_type = ccl::global_data::env().atl_transport;

    switch (transport_type) {
        case ccl_atl_ofi: atl_comm = std::shared_ptr<atl_base_comm>(new atl_ofi_comm(k)); break;
#ifdef CCL_ENABLE_MPI
        case ccl_atl_mpi: atl_comm = std::shared_ptr<atl_base_comm>(new atl_mpi_comm(k)); break;
#endif // CCL_ENABLE_MPI
        default: LOG_ERROR("Unsupported yet"); break;
    }
    return atl_comm;
}

std::shared_ptr<atl_base_comm> atl_comm_manager::create_comm(int total_rank_count,
                                                             const std::vector<int>& ranks,
                                                             std::shared_ptr<ikvs_wrapper> k) {
    std::shared_ptr<atl_base_comm> atl_comm;

    auto transport_type = ccl::global_data::env().atl_transport;

    switch (transport_type) {
        case ccl_atl_ofi:
            atl_comm = std::shared_ptr<atl_base_comm>(new atl_ofi_comm(total_rank_count, ranks, k));
            break;
#ifdef CCL_ENABLE_MPI
        case ccl_atl_mpi:
            atl_comm = std::shared_ptr<atl_base_comm>(new atl_mpi_comm(total_rank_count, ranks, k));
            break;
#endif // CCL_ENABLE_MPI
        default: LOG_ERROR("Unsupported yet"); break;
    }
    return atl_comm;
}

void atl_comm_manager::set_internal_env(const atl_attr_t& attr) {
    auto transport_type = ccl::global_data::env().atl_transport;
    atl_base_comm::attr = attr;

    if (transport_type == ccl_atl_ofi)
        atl_ofi::atl_set_env(attr);
#ifdef CCL_ENABLE_MPI
    else if (transport_type == ccl_atl_mpi)
        atl_mpi::atl_set_env(attr);
#endif // CCL_ENABLE_MPI
}

void atl_comm_manager::set_exec(ccl_executor* exec) {
    atl_base_comm::executor = exec;
}
