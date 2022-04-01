#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

void run_collective(const char* cmd_name,
                    const std::vector<float>& send_buf,
                    std::vector<float>& recv_buf,
                    const ccl::communicator& comm,
                    const ccl::allreduce_attr& attr) {
    float expected = (static_cast<float>(comm.size()) + 1) / 2 * static_cast<float>(comm.size());

    ccl::barrier(comm);

    for (size_t idx = 0; idx < ITERS; ++idx) {
        ccl::allreduce(
            send_buf.data(), recv_buf.data(), recv_buf.size(), ccl::reduction::sum, comm, attr)
            .wait();
    }

    for (size_t idx = 0; idx < recv_buf.size(); idx++) {
        if (recv_buf[idx] != expected) {
            fprintf(stderr, "idx %zu, expected %4.4f, got %4.4f\n", idx, expected, recv_buf[idx]);
            std::cout << "FAILED" << std::endl;
            std::terminate();
        }
    }

    ccl::barrier(comm);
}

int main(int argc, char* argv[]) {
    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    sycl::property_list props{ sycl::property::queue::in_order{} };
    queue q;
    if (!create_sycl_queue("gpu", rank, q, props)) {
        return -1;
    }

    /* create kvs */
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
        main_addr = kvs->get_address();
    }

    /* create communicator */
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());

    try {
        ccl::create_communicator(size, rank, dev, ctx, kvs);
        cout << "FAILED\n";
        return -1;
    }
    catch (...) {
        cout << "can not create device communicator\n";
    }

    auto comm = ccl::create_communicator(size, rank, kvs);
    auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();
    cout << "host communicator is created\n";

    MSG_LOOP(comm, std::vector<float> send_buf(msg_count, static_cast<float>(comm.rank() + 1));
             std::vector<float> recv_buf(msg_count);
             run_collective("regular allreduce", send_buf, recv_buf, comm, attr););
    cout << "PASSED\n";

    return 0;
}
