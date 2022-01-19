#include "base.hpp"

using namespace std;

int main(int argc, char const *argv[]) {
    ccl::init();
    int status = MPI_Init(nullptr, nullptr);
    if (status != MPI_SUCCESS) {
        throw std::runtime_error{ "Problem occurred during MPI init" };
    }

    int size = 0;
    int rank = 0;

    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::create_kvs(main_addr);
    }
    const size_t count = 10 * 1024 * 1024;
    for (int i = 0; i < 5; i++) {
        auto comm = ccl::create_communicator(size, rank, kvs);
        std::vector<int> send_buf(count, 1);
        std::vector<int> recv_buf(count, 0);
        ccl::allreduce(send_buf.data(), recv_buf.data(), count, ccl::reduction::sum, comm).wait();
    }

    if (rank == 0) {
        std::cout << "PASSED" << std::endl;
    }
    return 0;
}