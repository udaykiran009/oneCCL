
#include "sycl_base.hpp"

int main(int argc, char **argv) {
    int i = 0;
    int size = 0;
    int rank = 0;

    ccl_stream_type_t stream_type;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cl::sycl::queue q;
    if (create_sycl_queue(argc, argv, q, stream_type) != 0) {
        MPI_Finalize();
        return -1;
    }

    /* create USM polymorphis allocator */
    usm_polymorphic_allocator<int,
                              cl::sycl::usm::alloc::host,
                              cl::sycl::usm::alloc::device,
                              cl::sycl::usm::alloc::shared>
        allocator(q);

    /* default type of USM allocation is SHARED */
    cl::sycl::usm::alloc usm_alloc_type = cl::sycl::usm::alloc::shared;
    if (argc > 2) {
        usm_alloc_type = usm_alloc_type_from_string(argv[2]);
    }

    int *sendbuf = allocator.allocate(COUNT * size, usm_alloc_type);
    int *recvbuf = allocator.allocate(COUNT * size, usm_alloc_type);
    std::vector<size_t> send_counts(size, COUNT);
    std::vector<size_t> recv_counts(size, COUNT);

    /* create CCL internal KVS */
    ccl::init();

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

    /* create SYCL communicator */
    auto ctx = q.get_context();
    auto communcators = ccl::create_device_communicators(
        size,
        ccl::vector_class<ccl::pair_class<ccl::rank_t, cl::sycl::device>>{
            { rank, q.get_device() } },
        ctx,
        kvs);
    auto &comm = *communcators.begin();

    /* create SYCL stream */
    auto stream = ccl::create_stream(q);

    q.submit([&](handler &cgh) {
        cgh.parallel_for<class alltoall_test_sbuf_modify>(range<1>{ COUNT * size },
                                                          [=](item<1> id) {
                                                              size_t index = id[0];
                                                              sendbuf[index] = index / COUNT + 1;
                                                              recvbuf[index] = -1;
                                                          });
    });

    handle_exception(q);

    /* invoke ccl_alltoall on the CPU side */
    auto attr = ccl::create_operation_attr<ccl::alltoallv_attr>();
    ccl::alltoallv(sendbuf, send_counts, recvbuf, recv_counts, comm, stream, attr).wait();

    /* open recvbuf and check its correctness on the target device side */
    cl::sycl::buffer<int, 1> out_recv_buf(COUNT * size);
    q.submit([&](handler &cgh) {
        auto out_recv_buf_dev = out_recv_buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class alltoall_test_rbuf_check>(range<1>{ COUNT * size }, [=](item<1> id) {
            size_t index = id[0];
            if (recvbuf[index] != rank + 1) {
                out_recv_buf_dev[id] = -1;
            }
        });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto host_acc_rbuf_new = out_recv_buf.get_access<mode::read>();
        for (i = 0; i < COUNT * size; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED" << std::endl;
                break;
            }
        }
        if (i == COUNT * size) {
            cout << "PASSED" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}
