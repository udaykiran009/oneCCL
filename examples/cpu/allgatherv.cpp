#include "base.hpp"
#include "mpi.h"

void run_collective(const char* cmd_name,
                    std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    std::vector<size_t>& recv_counts,
                    const ccl::communicator& comm,
                    const ccl::allgatherv_attr& coll_attr,
                    bool use_vector_call) {
    std::chrono::system_clock::duration exec_time{ 0 };
    float expected = send_buf.size();
    float received;

    ccl::barrier(comm);

    for (size_t idx = 0; idx < ITERS; ++idx) {
        auto start = std::chrono::system_clock::now();
        auto req = ccl::allgatherv(
            send_buf.data(), send_buf.size(), recv_buf.data(), recv_counts, comm, coll_attr);
        req.wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++) {
        received = recv_buf[idx];
        if (received != expected) {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n", idx, expected, received);

            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    ccl::barrier(comm);

    std::cout << "avg time of " << cmd_name << ": "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS
              << ", us" << std::endl;
}

int main() {
    MPI_Init(NULL, NULL);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    ccl::init();

    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }

    auto comm = ccl::create_communicator(size, rank, kvs);
    auto coll_attr = ccl::create_operation_attr<ccl::allgatherv_attr>();

    MSG_LOOP(comm,
        std::vector<float> send_buf(msg_count, static_cast<float>(msg_count));
        std::vector<float> recv_buf(comm.size() * msg_count, 0);
        std::vector<size_t> recv_counts(comm.size(), msg_count);
        coll_attr.set<ccl::operation_attr_id::to_cache>(0);
        run_collective("warmup_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr, false);
        coll_attr.set<ccl::operation_attr_id::to_cache>(1);
        run_collective("persistent_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr, false);
        coll_attr.set<ccl::operation_attr_id::to_cache>(0);
        run_collective("regular_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr, false);
    );

    MPI_Finalize();
    return 0;
}
