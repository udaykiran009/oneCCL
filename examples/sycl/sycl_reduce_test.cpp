#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {

    const size_t count = 10 * 1024 * 1024;
    const size_t root_rank = 0;

    int i = 0;
    int size = 0;
    int rank = 0;

    buffer<int> send_buf(count);
    buffer<int> recv_buf(count);

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    queue q;
    if (!create_sycl_queue(argc, argv, q)) {
        MPI_Finalize();
        return -1;
    }

    /* create kvs */
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

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    {
        /* open send_buf and recv_buf and initialize them on the host side */
        host_accessor send_buf_acc(send_buf, write_only);
        host_accessor recv_buf_acc(recv_buf, write_only);

        for (i = 0; i < count; i++) {
            send_buf_acc[i] = rank;
            recv_buf_acc[i] = 0;
        }
    }

    /* open send_buf and modify it on the device side */
    q.submit([&](auto &h) {
        accessor send_buf_acc(send_buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            send_buf_acc[id] += 1;
        });
    });

    if (!handle_exception(q))
        return -1;

    /* invoke reduce */
    ccl::reduce(send_buf, recv_buf, count, ccl::reduction::sum, root_rank, comm, stream).wait();

    /* open recv_buf and check its correctness on the device side */
    q.submit([&](auto &h) {
        accessor recv_buf_acc(recv_buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            if (rank == root_rank) {
                if (recv_buf_acc[id] != size * (size + 1) / 2) {
                    recv_buf_acc[id] = -1;
                }
            }
            else {
                if (recv_buf_acc[id] != 0) {
                    recv_buf_acc[id] = -1;
                }
            }
        });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    {
        if (rank == root_rank) {
            host_accessor recv_buf_acc(recv_buf, read_only);
            for (i = 0; i < count; i++) {
                if (recv_buf_acc[i] == -1) {
                    cout << "FAILED for rank: " << rank << "\n";
                    break;
                }
            }
            if (i == count) {
                cout << "PASSED\n";
            }
        }
    }

    MPI_Finalize();

    return 0;
}
