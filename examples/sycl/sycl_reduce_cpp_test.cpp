
#include "sycl_base.hpp"

int main(int argc, char **argv) {
    int i = 0;
    int size = 0;
    int rank = 0;
    ccl_stream_type_t stream_type;

    cl::sycl::queue q;
    cl::sycl::buffer<int, 1> sendbuf(COUNT);
    cl::sycl::buffer<int, 1> recvbuf(COUNT);

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    if (create_sycl_queue(argc, argv, q, stream_type) != 0) {
        MPI_Finalize();
        return -1;
    }

    /* create CCL internal KVS */
    auto &env = ccl::environment::instance();
    (void)env;
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::address_type main_addr;
    if (rank == 0) {
        kvs = ccl::environment::instance().create_main_kvs();
        main_addr = kvs->get_address();
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void *)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::environment::instance().create_kvs(main_addr);
    }

    /* create SYCL communicator */
    auto ctx = q.get_context();
    auto communcators = ccl::environment::instance().create_device_communicators(
        size,
        ccl::vector_class<ccl::pair_class<ccl::rank_t, cl::sycl::device>>{
            { rank, q.get_device() } },
        ctx,
        kvs);
    auto &comm = *communcators.begin();

    /* create SYCL stream */
    auto stream = ccl::environment::instance().create_stream(q);

    {
        /* open sendbuf and recvbuf and initialize them on the CPU side */
        auto host_acc_sbuf = sendbuf.get_access<mode::write>();
        auto host_acc_rbuf = recvbuf.get_access<mode::write>();

        for (i = 0; i < COUNT; i++) {
            host_acc_sbuf[i] = rank;
            host_acc_rbuf[i] = 0;
        }
    }

    /* open sendbuf and modify it on the target device side */
    q.submit([&](cl::sycl::handler &cgh) {
        auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_sbuf_modify>(range<1>{ COUNT }, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
        });
    });

    handle_exception(q);

    /* invoke ccl_reduce on the CPU side */
    auto attr = ccl::environment::instance().create_operation_attr<ccl::reduce_attr>();
    comm.reduce(sendbuf, recvbuf, COUNT, ccl::reduction::sum, COLL_ROOT, stream, attr)->wait();

    /* open recvbuf and check its correctness on the target device side */
    q.submit([&](handler &cgh) {
        auto dev_acc_rbuf = recvbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allreduce_test_rbuf_check>(range<1>{ COUNT }, [=](item<1> id) {
            if (rank == COLL_ROOT) {
                if (dev_acc_rbuf[id] != size * (size + 1) / 2) {
                    dev_acc_rbuf[id] = -1;
                }
            }
            else {
                if (dev_acc_rbuf[id] != 0) {
                    dev_acc_rbuf[id] = -1;
                }
            }
        });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    auto host_acc_rbuf_new = recvbuf.get_access<mode::read>();
    if (rank == COLL_ROOT) {
        for (i = 0; i < COUNT; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED for rank: " << rank << std::endl;
                break;
            }
        }
        if (i == COUNT) {
            cout << "PASSED" << std::endl;
        }
    }
    MPI_Finalize();
    return 0;
}
