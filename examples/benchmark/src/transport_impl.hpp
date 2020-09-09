#include <mpi.h>
#include "base.hpp"
#include "transport.hpp"

transport_settings::transport_settings()
{
    init_by_mpi();
}

transport_settings::~transport_settings()
{
    deinit_by_mpi();
}

transport_settings& transport_settings::instance()
{
    static transport_settings inst;
    return inst;
}

const int transport_settings::get_rank() const noexcept
{
    return rank;
}

const int transport_settings::get_size() const noexcept
{
    return size;
}

ccl::shared_ptr_class<ccl::kvs> transport_settings::get_kvs()
{
    return kvs;
}

void transport_settings::init_by_mpi()
{
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

     /* create CCL internal KVS */
    auto &env = ccl::environment::instance();
    (void)env;
    ccl::shared_ptr_class<ccl::kvs> kvs_candidate;
    ccl::kvs::addr_t master_addr;
    if (rank == 0)
    {
        kvs_candidate = ccl::environment::instance().create_main_kvs();
        master_addr = kvs_candidate->get_addr();
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs_candidate = ccl::environment::instance().create_kvs(master_addr);
    }
    kvs = kvs_candidate;
}

void transport_settings::deinit_by_mpi()
{
    MPI_Finalize();
}
