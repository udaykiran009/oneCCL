#include "base.hpp"

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    const ccl::communicator& comm,
                    const ccl::reduce_scatter_attr& attr) {
    std::chrono::system_clock::duration exec_time{ 0 };
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    ccl::barrier(comm);

    for (size_t idx = 0; idx < ITERS; ++idx) {
        auto start = std::chrono::system_clock::now();
        ccl::reduce_scatter(
            send_buf.data(), recv_buf.data(), recv_buf.size(), ccl::reduction::sum, comm, attr)
            .wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++) {
        if (recv_buf[idx] != expected) {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n", idx, expected, recv_buf[idx]);

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
    ccl::init();

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

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
    auto attr = ccl::create_operation_attr<ccl::reduce_scatter_attr>();

    MSG_LOOP(comm,
             std::vector<float> send_buf(msg_count * comm.size(), static_cast<float>(comm.rank()));
             std::vector<float> recv_buf(msg_count);
             attr.set<ccl::operation_attr_id::to_cache>(false);
             run_collective("warmup reduce_scatter", send_buf, recv_buf, comm, attr);
             attr.set<ccl::operation_attr_id::to_cache>(true);
             run_collective("persistent reduce_scatter", send_buf, recv_buf, comm, attr);
             attr.set<ccl::operation_attr_id::to_cache>(false);
             run_collective("regular reduce_scatter", send_buf, recv_buf, comm, attr););

    return 0;
}
