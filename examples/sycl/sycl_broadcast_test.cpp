#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {

    const size_t count = 10 * 1024 * 1024;

    int i = 0;
    int size = 0;
    int rank = 0;

    queue q;
    buffer<int> buf(count);

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

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
        /* open buf and initialize it on the host side */
        host_accessor send_buf_acc(buf, write_only);
        for (i = 0; i < count; i++) {
            if (rank == COLL_ROOT)
                send_buf_acc[i] = rank;
            else
                send_buf_acc[i] = 0;
        }
    }

    /* open buf and modify it on the device side */
    q.submit([&](auto &h) {
        accessor send_buf_acc(buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            send_buf_acc[id] += 1;
        });
    });

    if (!handle_exception(q))
        return -1;

    /* invoke broadcast */
    ccl::broadcast(buf, count, COLL_ROOT, comm, stream).wait();

    /* open buf and check its correctness on the device side */
    q.submit([&](auto &h) {
        accessor recv_buf_acc(buf, h, write_only);
        h.parallel_for(count, [=](auto id) {
            if (recv_buf_acc[id] != COLL_ROOT + 1) {
                recv_buf_acc[id] = -1;
            }
        });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    if (rank == COLL_ROOT) {
        host_accessor recv_buf_acc(buf, read_only);
        for (i = 0; i < count; i++) {
            if (recv_buf_acc[i] == -1) {
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
