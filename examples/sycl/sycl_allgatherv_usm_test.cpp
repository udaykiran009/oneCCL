#include "sycl_base.hpp"

int main(int argc, char **argv) {
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    cl::sycl::queue q;
    if (create_sycl_queue(argc, argv, q) != 0) {
        MPI_Finalize();
        return -1;
    }

    buf_allocator<int> allocator(q);

    cl::sycl::usm::alloc usm_alloc_type = cl::sycl::usm::alloc::shared;
    if (argc > 2) {
        usm_alloc_type = usm_alloc_type_from_string(argv[2]);
    }

    int* sendbuf = allocator.allocate(COUNT, usm_alloc_type);
    int* recvbuf = allocator.allocate(COUNT * size, usm_alloc_type);

    cl::sycl::buffer<int, 1> expected_buf(COUNT * size);
    cl::sycl::buffer<int, 1> out_recv_buf(COUNT * size);

    /* create CCL internal KVS */
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
    ccl::device dev = ccl::create_device(q.get_device());
    ccl::context ccl_ctx = ccl::create_context(q.get_context());
    auto communicators = ccl::create_communicators(
        size,
        ccl::vector_class<ccl::pair_class<ccl::rank_t, cl::sycl::device>>{
            { rank, dev.get_native() } },
        ccl_ctx.get_native(),
        kvs);
    auto &comm = *communicators.begin();

    /* create SYCL stream */
    auto stream = ccl::create_stream(q);

    /* create SYCL event */
    cl::sycl::event e;
    std::vector<ccl::event> events_list;
    // events_list.push_back(ccl::create_event(e));

    std::vector<size_t> recv_counts(size, COUNT);

    /* open sendbuf and modify it on the target device side */
    q.submit([&](handler &cgh) {
        auto expected_acc_buf_dev = expected_buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_sbuf_modify>(range<1>{ COUNT }, [=](item<1> id) {
            size_t index = id[0];
            sendbuf[index] = rank + 1;
            recvbuf[index] = -1;
            for (int i = 0; i < size; i++) {
                expected_acc_buf_dev[i * COUNT + index] = i + 1;
            }
        });
    });

    handle_exception(q);

    /* invoke ccl_allagtherv on the CPU side */
    auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
    ccl::allgatherv(sendbuf, COUNT, recvbuf, recv_counts, comm, stream, attr, events_list).wait();

    /* open recvbuf and check its correctness on the target device side */
    q.submit([&](handler &cgh) {
        auto expected_acc_buf_dev = expected_buf.get_access<mode::read>(cgh);
        auto out_recv_buf_dev = out_recv_buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_rbuf_check>(
            range<1>{ size * COUNT }, [=](item<1> id) {
                size_t index = id[0];
                if (recvbuf[index] != expected_acc_buf_dev[index]) {
                    out_recv_buf_dev[index] = -1;
                }
            });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto control_buf_accessed = out_recv_buf.get_access<mode::read>();
        int i = 0;
        for (i = 0; i < size * COUNT; i++) {
            if (control_buf_accessed[i] == -1) {
                cout << "FAILED";
                break;
            }
        }
        if (i == size * COUNT) {
            cout << "PASSED" << std::endl;
        }
    }

    MPI_Finalize();
    return 0;
}
