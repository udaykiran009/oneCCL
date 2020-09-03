
#include "sycl_base.hpp"

int main(int argc, char **argv)
{
    int i = 0;
    int j = 0;
    int size = 0;
    int rank = 0;
    ccl_stream_type_t stream_type;

    cl::sycl::queue q;

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);


    cl::sycl::buffer<int, 1> sendbuf(COUNT);
    cl::sycl::buffer<int, 1> expected_buf(COUNT * size);
    cl::sycl::buffer<int, 1> recvbuf(size * COUNT);

    if (create_sycl_queue(argc, argv, q, stream_type) != 0) {
        return -1;
    }

    /* create CCL internal KVS */
    ccl::shared_ptr_class<ccl::kvs> kvs;
    ccl::kvs::addr_t master_addr;
    if (rank == 0)
    {
        kvs = ccl::environment::instance().create_main_kvs();
        master_addr = kvs->get_addr();
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else
    {
        MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs = ccl::environment::instance().create_kvs(master_addr);
    }

    /* create SYCL communicator */
    auto comm = ccl::environment::instance().create_single_device_communicator(size, rank, q, kvs);

    /* create SYCL stream */
    auto stream = ccl::environment::instance().create_stream(q);

    /* create SYCL event */
    cl::sycl::event e;
    std::vector<ccl::event> events_list;
    events_list.push_back(ccl::environment::instance().create_event(e));

    std::vector<size_t> recv_counts(size, COUNT);

    {
        /* open buffers and initialize them on the CPU side */
        auto host_acc_sbuf = sendbuf.get_access<mode::write>();
        auto host_acc_rbuf = recvbuf.get_access<mode::write>();
        auto expected_acc_buf = expected_buf.get_access<mode::write>();

        for (i = 0; i < COUNT; i++) {
            host_acc_sbuf[i] = rank;
        }
        for (i = 0; i < COUNT * size; i++) {
            host_acc_rbuf[i] = -1;
        }
        for (i = 0; i < size; i++) {
            for (j = 0; j < COUNT; j++) {
                expected_acc_buf[i * COUNT + j] = i + 1;
            }
        }
    }

    /* open sendbuf and modify it on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
        cgh.parallel_for<class allgatherv_test_sbuf_modify>(range<1>{COUNT}, [=](item<1> id) {
            dev_acc_sbuf[id] += 1;
        });
    });

    handle_exception(q);

    /* invoke ccl_allagtherv on the CPU side */
    auto attr = ccl::environment::instance().create_op_attr<ccl::allgatherv_attr_t>();
    comm.allgatherv(sendbuf,
                    COUNT,
                    recvbuf,
                    recv_counts,
                    attr,
                    stream,
                    events_list)->wait();

    /* open recvbuf and check its correctness on the target device side */
    q.submit([&](handler& cgh) {
        auto dev_acc_rbuf = recvbuf.get_access<mode::write>(cgh);
        auto expected_acc_buf_dev = expected_buf.get_access<mode::read>(cgh);
        cgh.parallel_for<class allgatherv_test_rbuf_check>(range<1>{size * COUNT}, [=](item<1> id) {
            if (dev_acc_rbuf[id] != expected_acc_buf_dev[id]) {
                dev_acc_rbuf[id] = -1;
            }
        });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT) {
        auto host_acc_rbuf_new = recvbuf.get_access<mode::read>();
        for (i = 0; i < size * COUNT; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout << "FAILED" ;
                break;
            }
        }
        if (i == size * COUNT) {
            cout << "PASSED" << std::endl;
        }
    }

    return 0;
}
