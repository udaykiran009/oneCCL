#include "base.hpp"
#include "mpi.h"

void run_collective(const char* cmd_name,
                    std::vector<float>& buf,
                    ccl::communicator& comm,
                    ccl::bcast_attr_t& coll_attr)
{
    std::chrono::system_clock::duration exec_time{0};
    float received;

    if (comm.rank() == COLL_ROOT)
    {
        for (size_t idx = 0; idx < buf.size(); idx++)
        {
            buf[idx] = static_cast<float>(idx);
        }
    }
    comm.barrier();

    for (size_t idx = 0; idx < ITERS; ++idx)
    {
        auto start = std::chrono::system_clock::now();
        auto req = comm.bcast(buf.data(),
                              buf.size(),
                              COLL_ROOT,
                              coll_attr);
        req->wait();
        exec_time += std::chrono::system_clock::now() - start;
    }

    for (size_t idx = 0; idx < buf.size(); idx++)
    {
        received = buf[idx];
        if (received != idx)
        {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n",
                    idx, static_cast<float>(idx), received);

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

    auto &env = ccl::environment::instance();
    (void)env;

    /* create CCL internal KVS */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::addr_t master_addr;
    if (rank == 0)
    {
        kvs = env.create_main_kvs();
        master_addr = kvs->get_addr();
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = env.create_kvs(master_addr);
    }

    auto comm = env.create_communicator(size, rank, kvs);
    auto coll_attr = ccl::environment::instance().create_op_attr<ccl::bcast_attr_t>();

    MSG_LOOP(comm,
        std::vector<float> buf(msg_count);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("warmup_bcast", buf, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(1);
        run_collective("persistent_bcast", buf, comm, coll_attr);
        coll_attr.set<ccl::common_op_attr_id::to_cache>(0);
        run_collective("regular_bcast", buf, comm, coll_attr);
    );

    MPI_Finalize();
    return 0;
}
