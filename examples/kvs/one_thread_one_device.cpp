
#include "base.hpp"

#define COUNT     512 //(10*1024*1024)
#define COLL_ROOT (0)

using processing_type = float;
using processing_type_ptr = float*;

#ifdef CCL_ENABLE_SYCL
void run_test(const size_t size,
              const size_t device_id,
              std::shared_ptr<ccl::ccl_kvs_interface> kvs) {
    using namespace ::native;

    ccl::device_communicator_t comm =
        ccl::environment::instance().create_device_communicators(size, { device_id }, kvs)[0];
    size_t rank = comm->rank();
    ccl::communicator::device_native_reference_t dev = comm->get_device();

    cl::sycl::queue queue(dev);

    // allocate memory
    processing_type* mem_send = static_cast<processing_type*>(
        cl::sycl::aligned_alloc_device(sizeof(processing_type), COUNT, queue));
    processing_type* mem_recv = static_cast<processing_type*>(
        cl::sycl::aligned_alloc_device(sizeof(processing_type), COUNT, queue));

    if (!mem_send or !mem_recv) {
        std::cerr << "Cannot allocate USM! Abort" << std::endl;
        abort();
    }

    // set initial memory
    std::iota(mem_send, mem_send + COUNT, 1);
    std::fill(mem_recv, mem_recv + COUNT, 0);

    ccl::stream_t stream = ccl::environment::instance().create_stream(queue);

    //allreduce
    ccl::attr attr{};

    ccl::event req =
        comm->allreduce(mem_send, mem_recv, COUNT, ccl::reduction::sum, &attr, stream);

    //wait
    req.wait();
}
#else
void run_test(const size_t size,
              const size_t device_id,
              std::shared_ptr<ccl::ccl_kvs_interface> kvs) {
    using namespace ::native;
    ccl::device_communicator_t comm =
        ccl::environment::instance().create_device_communicators(size, { device_id }, kvs)[0];
    size_t rank = comm->rank();
    ccl::communicator::device_native_reference_t dev = comm->get_device();

    std::vector<processing_type> send_values(COUNT);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<processing_type> recv_values(COUNT, 0);

    // wrapped L0-native API for devices: create native buffers
    auto mem_send = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));
    auto mem_recv = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));

    // wrapped L0-native API for memory: fill device buffers
    mem_send.enqueue_write_sync(send_values);
    mem_recv.enqueue_write_sync(recv_values);

    // create native stream

    ccl_device::device_queue queue = dev->create_cmd_queue();

    ccl::stream_t stream = ccl::environment::instance().create_stream(queue);

    //allreduce
    ccl::attr attr{};

    ccl::event req =
        comm->allreduce(mem_send, mem_recv, COUNT, ccl::reduction::sum, &attr, stream);

    //wait
    req.wait();
}
#endif // CCL_ENABLE_SYCL
int main(int argc, char** argv) {
    using namespace ::native;

    // TODO: add logic for device_id
    size_t size;
    size_t device_id;
    int rank, size;

    //initializing MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::shared_ptr<ccl::ccl_internal_kvs> kvs;
    ccl::ccl_main_addr main_addr;
    if (rank == 0) {
        kvs = std::make_shared<ccl::ccl_internal_kvs>();
        main_addr = kvs->get_main_addr();
        MPICHECK(MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD));
    }
    else {
        MPICHECK(MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD));
        kvs = std::make_shared<ccl::ccl_internal_kvs>(main_addr);
    }

    run_test(size, device_id, kvs)

        MPI_Finalize();

    return 0;
}