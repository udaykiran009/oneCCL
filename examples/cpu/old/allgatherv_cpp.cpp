#include "base.hpp"
#include "mpi.h"

void run_collective(const char* cmd_name,
                    std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    std::vector<size_t>& recv_counts,
                    ccl::communicator& comm,
                    ccl::allgatherv_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float expected = send_buf.size();
    float received;

    comm.barrier();

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        auto start = std::chrono::system_clock::now();
        auto req = comm.allgatherv(send_buf.data(),
                                   send_buf.size(),
                                   recv_buf.data(),
                                   recv_counts,
                                   coll_attr);
        req->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++)
    {
        received = recv_buf[idx];
        if (received != expected)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, expected, received);

            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    comm.barrier();

    std::cout << "avg time of " << cmd_name << ": "
              << std::chrono::duration_cast<std::chrono::microseconds>(exec_time).count() / ITERS
              << ", us" << std::endl;
}

int main()
{
    MPI_Init(NULL, NULL);
    int size, rank;
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    
    /* create CCL internal KVS */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::addr_t master_addr;
    if (rank == 0)
    {
        kvs = ccl::environment::instance().create_main_kvs();
        master_addr = kvs->get_addr();
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::environment::instance().create_kvs(master_addr);
    }

    auto comm = ccl::environment::instance().create_communicator(size, rank, kvs);
    auto coll_attr = ccl::environment::instance().create_op_attr<ccl::allgatherv_attr_t>();

    MSG_LOOP(comm,
        std::vector<float> send_buf(msg_count, static_cast<float>(msg_count));
        std::vector<float> recv_buf(comm.size() * msg_count, 0);
        std::vector<size_t> recv_counts(comm.size(), msg_count);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("warmup_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(1);
        run_collective("persistent_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("regular_allgatherv", send_buf, recv_buf, recv_counts, comm, coll_attr);
    );

    MPI_Finalize();
    return 0;
}
