#include "base.hpp"

using namespace std;

int main(int argc, char const *argv[]) {
    const size_t count = 10 * 1024;

    ccl::init();

    int status = MPI_Init(nullptr, nullptr);
    if (status != MPI_SUCCESS) {
        throw std::runtime_error{ "problem occurred during MPI init" };
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

    for (int i = 0; i < 20; i++) {
        if (rank == 0) {
            std::cout << "start iter " << i << "\n";
        }

        auto comm = ccl::create_communicator(size, rank, kvs);
        std::vector<float> send_buf(count, static_cast<float>(comm.rank() + 1));
        std::vector<float> recv_buf(count, 0);

        ccl::allreduce(send_buf.data(), recv_buf.data(), count, ccl::reduction::sum, comm).wait();

        float expected =
            (static_cast<float>(comm.size()) + 1) / 2 * static_cast<float>(comm.size());
        if (std::any_of(recv_buf.begin(), recv_buf.end(), [expected](float value) {
                return value != expected;
            })) {
            std::cout << "FAILED" << std::endl;
            return -1;
        }
    }

    if (rank == 0) {
        std::cout << "PASSED" << std::endl;
    }

    return 0;
}
