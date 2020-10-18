#include <mpi.h>

#ifdef CCL_ENABLE_SYCL
#include "sycl_coll.hpp"
#endif /* CCL_ENABLE_SYCL */

#include "base.hpp"
#include "transport.hpp"

transport_data::transport_data() {
    init_by_mpi();
}

transport_data::~transport_data() {
    deinit_by_mpi();
}

transport_data& transport_data::instance() {
    static transport_data inst;
    return inst;
}

size_t transport_data::get_comm_size() {
    return transport_data::instance().get_host_comms()[0].size();
}

int transport_data::get_rank() const noexcept {
    return rank;
}

int transport_data::get_size() const noexcept {
    return size;
}

ccl::shared_ptr_class<ccl::kvs> transport_data::get_kvs() {
    return kvs;
}

void transport_data::init_by_mpi() {

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ccl::shared_ptr_class<ccl::kvs> kvs_candidate;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs_candidate = ccl::create_main_kvs();
        main_addr = kvs_candidate->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs_candidate = ccl::create_kvs(main_addr);
    }
    kvs = kvs_candidate;
}

void transport_data::deinit_by_mpi() {
    MPI_Finalize();
}

void transport_data::init_host_comms(size_t ranks_per_proc) {
    
    streams.clear();

    std::map<size_t, ccl::device> r2d_map;
    for (size_t idx = 0; idx < ranks_per_proc; idx++) {
        streams.push_back(ccl::create_stream());
        r2d_map.emplace(rank * ranks_per_proc + idx, ccl::create_device());
    }

    auto context = ccl::create_context();
    host_comms = ccl::create_communicators(size, r2d_map, context, kvs);

    ASSERT(host_comms.size() == ranks_per_proc,
        "unexpected host_comms size %zu, expected %zu", host_comms.size(), ranks_per_proc);
}

std::vector<ccl::communicator>& transport_data::get_host_comms() {
    return host_comms;
}

std::vector<ccl::stream>& transport_data::get_streams() {
    return streams;
}

#ifdef CCL_ENABLE_SYCL

#include <CL/sycl.hpp>

void transport_data::init_device_comms(size_t ranks_per_proc) {

    std::vector<size_t> local_ranks;
    for (size_t idx = 0; idx < ranks_per_proc; idx++) {
        local_ranks.push_back(rank * ranks_per_proc + idx);
    }

    auto sycl_queues = get_sycl_queues(local_ranks);

    ASSERT(!sycl_queues.empty(), "queues should contain at least one queue");
    ASSERT(ranks_per_proc == sycl_queues.size(), "ranks and queues sizes should match");
      
    auto sycl_context = sycl_queues[0].get_context();
    auto context = ccl::create_context(sycl_context);

    std::vector<ccl::device> devices;
    streams.clear();

    std::map<size_t, ccl::device> r2d_map;
    for (size_t idx = 0; idx < ranks_per_proc; idx++) {
        streams.push_back(ccl::create_stream(sycl_queues[idx]));
        r2d_map.emplace(local_ranks[idx], ccl::create_device(sycl_queues[idx].get_device()));
        ASSERT(sycl_context == sycl_queues[idx].get_context(),
            "all sycl queues should be from the same sycl context");
    }

    device_comms = ccl::create_communicators(size, r2d_map, context, kvs);

     ASSERT(device_comms.size() == ranks_per_proc,
        "unexpected device_comms size %zu, expected %zu", device_comms.size(), ranks_per_proc);
}

std::vector<ccl::communicator>& transport_data::get_device_comms() {
    return device_comms;
}

#endif /* CCL_ENABLE_SYCL */
