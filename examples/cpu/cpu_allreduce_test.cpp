#include <iostream>
#include <mpi.h>

#include "oneapi/ccl.hpp"

using namespace std;

int main() {
    const size_t count = 4096;

    size_t i = 0;

    int send_buf[count];
    int recv_buf[count];

    ccl::init();

    int size, rank;
    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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

    rank = comm.rank();
    size = comm.size();

    /* initialize send_buf */
    for (i = 0; i < count; i++) {
        send_buf[i] = rank;
    }

    /* modify send_buf */
    for (i = 0; i < count; i++) {
        send_buf[i] += 1;
    }

    /* invoke allreduce */
    ccl::allreduce(send_buf, recv_buf, count, ccl::reduction::sum, comm).wait();

    /* check correctness of recv_buf */
    for (i = 0; i < count; i++) {
        if (recv_buf[i] != size * (size + 1) / 2) {
            recv_buf[i] = -1;
        }
    }

    /* print out the result of the test */
    if (rank == 0) {
        for (i = 0; i < count; i++) {
            if (recv_buf[i] == -1) {
                cout << "FAILED\n";
                break;
            }
        }
        if (i == count) {
            cout << "PASSED\n";
        }
    }

    MPI_Finalize();

    return 0;
}
