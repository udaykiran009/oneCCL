
#include "sycl_base.hpp"

using native_data_t = int32_t;

struct custom_data_type {
    native_data_t arr[sizeof(uint32_t)];
} __attribute__((packed));

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

    constexpr size_t actual_data_count = COUNT * sizeof(custom_data_type) / sizeof(native_data_t);
    cl::sycl::buffer<native_data_t, 1> expected_buf(actual_data_count * size);
    cl::sycl::buffer<native_data_t, 1> out_recv_buf(actual_data_count * size);

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
    auto comm = ccl::create_communicator( size, rank, dev, ctx, kvs);

    /* create stream */
    auto stream = ccl::create_stream(q);

    /* create event */
    cl::sycl::event e;
    std::vector<ccl::event> events_list;
    // events_list.push_back(ccl::create_event(e));

    std::vector<size_t> recv_counts(size, actual_data_count);

    /* open sendbuf and modify it on the target device side */
    e = q.submit([&](handler &cgh) {
        auto expected_acc_buf_dev = expected_buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_sbuf_modify>(
            range<1>{ actual_data_count }, [=](item<1> id) {
                size_t index = id[0];
                static_cast<native_data_t *>(sendbuf)[index] = rank + 1;
                static_cast<native_data_t *>(recvbuf)[index] = -1;
                for (int i = 0; i < size; i++) {
                    expected_acc_buf_dev[i * actual_data_count + index] = i + 1;
                }
            });
    });

    handle_exception(q);

    /* invoke allagtherv */
    auto attr = ccl::create_operation_attr<ccl::allgatherv_attr>();
    ccl::allgatherv(sendbuf,
                    actual_data_count,
                    recvbuf,
                    recv_counts,
                    ccl::datatype::int32,
                    comm,
                    stream,
                    attr,
                    events_list)
        .wait();

    /* open recvbuf and check its correctness on the target device side */
    q.submit([&](handler &cgh) {
        auto expected_acc_buf_dev = expected_buf.get_access<mode::read>(cgh);
        auto out_recv_buf_dev = out_recv_buf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_rbuf_check>(
            range<1>{ size * actual_data_count }, [=](item<1> id) {
                size_t index = id[0];
                if (static_cast<native_data_t *>(recvbuf)[index] != expected_acc_buf_dev[index]) {
                    out_recv_buf_dev[index] = -1;
                }
            });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto control_buf_accessed = out_recv_buf.get_access<mode::read>();
        int i = 0;
        for (i = 0; i < size * actual_data_count; i++) {
            if (control_buf_accessed[i] == -1) {
                cout << "FAILED";
                break;
            }
        }
        if (i == size * actual_data_count) {
            cout << "PASSED" << std::endl;
        }
    }

    MPI_Finalize();

    return 0;
}
