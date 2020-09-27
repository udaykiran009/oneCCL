#include <iostream>
#include <mpi.h>

#include "oneapi/ccl.hpp"

#define COUNT 128

using namespace std;

int main() {

    int i = 0;

    int send_buf[COUNT];
    int recv_buf[COUNT];

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
    for (i = 0; i < COUNT; i++) {
        send_buf[i] = rank;
    }

    /* modify send_buf */
    for (i = 0; i < COUNT; i++) {
        send_buf[i] += 1;
    }

    /* invoke ccl_allreduce */
    ccl::allreduce(send_buf,
                   recv_buf,
                   COUNT,
                   ccl::reduction::sum,
                   comm).wait();

    /* check correctness of recv_buf */
    for (i = 0; i < COUNT; i++) {
        if (recv_buf[i] != size * (size + 1) / 2) {
            recv_buf[i] = -1;
        }
    }

    /* print out the result of the test */
    if (rank == 0) {
        for (i = 0; i < COUNT; i++) {
            if (recv_buf[i] == -1) {
                cout << "FAILED" << endl;
                break;
            }
        }
        if (i == COUNT) {
            cout << "PASSED" << endl;
        }
    }

    MPI_Finalize();

    return 0;
}
