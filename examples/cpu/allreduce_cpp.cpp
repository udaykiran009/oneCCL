#include "base.hpp"
#include "mpi.h"

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    ccl::communicator& comm,
                    ccl::allreduce_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float expected = (comm.size() - 1) * (static_cast<float>(comm.size()) / 2);

    comm.barrier();

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        auto start = std::chrono::system_clock::now();
        auto req = comm.allreduce(send_buf.data(),
                                  recv_buf.data(),
                                  recv_buf.size(),
                                  ccl_reduction_sum,
                                  coll_attr);
        req->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++)
    {
        if (recv_buf[idx] != expected)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, expected, recv_buf[idx]);

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
    auto kvs = ccl::environment::instance().create_main_kvs();
    auto comm = ccl::environment::instance().create_communicator(size, rank, kvs);
    ccl::allreduce_attr_t coll_attr = ccl::default_allreduce_attr;

    MSG_LOOP(comm,
        std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank()));
        std::vector<float> recv_buf(msg_count);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("warmup allreduce", send_buf, recv_buf, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(1);
        run_collective("persistent allreduce", send_buf, recv_buf, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("regular allreduce", send_buf, recv_buf, comm, coll_attr);
    );

    MPI_Finalize();
    return 0;
}
