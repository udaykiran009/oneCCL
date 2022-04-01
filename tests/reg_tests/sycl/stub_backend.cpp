#include "sycl_base.hpp"

using namespace std;
using namespace sycl;

int main(int argc, char *argv[]) {
    const size_t count = 10 * 1024 * 1024;

    int size = 0;
    int rank = 0;

    ccl::init();

    MPI_Init(NULL, NULL);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    atexit(mpi_finalize);

    queue q;
    if (!create_sycl_queue("gpu", rank, q)) {
        return -1;
    }

    buf_allocator<int> allocator(q);

    // create kvs
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

    // create communicator
    auto dev = ccl::create_device(q.get_device());
    auto ctx = ccl::create_context(q.get_context());
    auto comm = ccl::create_communicator(size, rank, dev, ctx, kvs);

    // create stream
    auto stream = ccl::create_stream(q);

    auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();

    auto *send_buf = allocator.allocate(count, usm::alloc::device);
    auto *recv_buf = allocator.allocate(count, usm::alloc::device);

    // note: stub backend support only version with void* and datatype parameter
    ccl::allreduce((void *)send_buf,
                   (void *)recv_buf,
                   count,
                   ccl::datatype::int32,
                   ccl::reduction::sum,
                   comm,
                   stream,
                   attr)
        .wait();

    q.wait();

    allocator.deallocate(send_buf);
    allocator.deallocate(recv_buf);

    std::cout << "PASSED" << std::endl;

    return 0;
}
