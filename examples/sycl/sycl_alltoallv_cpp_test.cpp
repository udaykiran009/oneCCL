
#include "sycl_base.hpp"

int main(int argc, char **argv)
{
    int i = 0;
    size_t size = 0;
    size_t rank = 0;
    size_t* send_counts;
    size_t* recv_counts;

    auto comm = ccl::environment::instance().create_communicator();

    rank = comm->rank();
    size = comm->size();

    cl::sycl::queue q;
    cl::sycl::buffer<int, 1> sendbuf(COUNT * size);
    cl::sycl::buffer<int, 1> recvbuf(COUNT * size);

    if (create_sycl_queue(argc, argv, q) != 0) {
        return -1;
    }
    /* create SYCL stream */
    auto stream = ccl::environment::instance().create_stream(ccl::stream_type::device, &q);

    send_counts = static_cast<size_t*>(malloc(size * sizeof(size_t)));
    recv_counts = static_cast<size_t*>(malloc(size * sizeof(size_t)));

    for (size_t idx = 0; idx < size; idx++) {
        send_counts[idx] = COUNT;
        recv_counts[idx] = COUNT;
    }

    {
        /* open buffers and initialize them on the CPU side */
        auto host_acc_sbuf = sendbuf.get_access<mode::write>();
        auto host_acc_rbuf = recvbuf.get_access<mode::write>();

        for (int i = 0; i < size; i++) {
            for (int j = 0; j < COUNT; j++) {
                host_acc_sbuf[(i * COUNT) + j] = i;
                host_acc_rbuf[(i * COUNT) + j] = -1;
            }
        }
    }

    /* open sendbuf and modify it on the target device side */
    q.submit([&](handler& cgh){
       auto dev_acc_sbuf = sendbuf.get_access<mode::write>(cgh);
       cgh.parallel_for<class alltoallv_test_sbuf_modify>(range<1>{COUNT * size}, [=](item<1> id) {
           dev_acc_sbuf[id] += 1;
       });
    });

    handle_exception(q);

    /* invoke ccl_alltoall on the CPU side */
    comm->alltoallv(sendbuf,
                    send_counts,
                    recvbuf,
                    recv_counts,
                    nullptr, /* attr */
                    stream)->wait();

    /* open recvbuf and check its correctness on the target device side */
    q.submit([&](handler& cgh){
       auto dev_acc_rbuf = recvbuf.get_access<mode::write>(cgh);
       cgh.parallel_for<class alltoallv_test_rbuf_check>(range<1>{COUNT * size}, [=](item<1> id) {
           if (dev_acc_rbuf[id] != rank + 1) {
               dev_acc_rbuf[id] = -1;
           }
       });
    });

    handle_exception(q);

    /* print out the result of the test on the CPU side */
    if (rank == COLL_ROOT){
        auto host_acc_rbuf_new = recvbuf.get_access<mode::read>();
        for (i = 0; i < COUNT * size; i++) {
            if (host_acc_rbuf_new[i] == -1) {
                cout<<"FAILED"<< std::endl;
                break;
            }
        }
        if (i == COUNT * size) {
            cout<<"PASSED"<< std::endl;
        }
    }

    free(send_counts);
    free(recv_counts);

    return 0;
}
