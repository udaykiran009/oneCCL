#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {

    const size_t count = 10 * 1024 * 1024;

    int i = 0;
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    queue q;
    if (!create_sycl_queue(argc, argv, q)) {
        MPI_Finalize();
        return -1;
    }

    buf_allocator<int> allocator(q);

    auto usm_alloc_type = usm::alloc::shared;
    if (argc > 2) {
        usm_alloc_type = usm_alloc_type_from_string(argv[2]);
    }

    auto send_buf = allocator.allocate(count * size, usm_alloc_type);
    auto recv_buf = allocator.allocate(count * size, usm_alloc_type);

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

    /* open send_buf and modify it on the device side */
    q.submit([&](auto &h) {
        h.parallel_for(count * size,
                                                          [=](auto id) {
                                                              send_buf[id] = id / count + 1;
                                                              recv_buf[id] = -1;
                                                          });
    });

    if (!handle_exception(q))
        return -1;

    /* invoke alltoall */
    ccl::alltoall(send_buf, recv_buf, count, comm, stream).wait();

    /* open recv_buf and check its correctness on the device side */
    buffer<int> check_buf(count * size);
    q.submit([&](auto &h) {
        accessor check_buf_acc(check_buf, h, write_only);
        h.parallel_for(count * size, [=](auto id) {
            if (recv_buf[id] != rank + 1) {
                check_buf_acc[id] = -1;
            }
        });
    });

    if (!handle_exception(q))
        return -1;

    /* print out the result of the test on the host side */
    {
        host_accessor check_buf_acc(check_buf, read_only);
        for (i = 0; i < count * size; i++) {
            if (check_buf_acc[i] == -1) {
                cout << "FAILED\n";
                break;
            }
        }
        if (i == count * size) {
            cout << "PASSED\n";
        }
    }

    MPI_Finalize();

    return 0;
}
