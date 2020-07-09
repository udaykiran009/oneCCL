#define COUNT     512
#define ALIGNMENT 64

using dtype = float;
using dtype_ptr = float*;

int main(int argc, char** argv) {

    /* Note: SYCL devices are provided by framework */
    std::vector<sycl::device> devices;

    /* initialize MPI to bcast master kvs address */
    MPI_Init(&argc, &argv);

    int mpi_rank, mpi_size;
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);

    /* create key-value store */

    auto env = ccl::environment::instance();
    ccl::kvs_interface_t kvs;
    ccl::master_kvs::addr_t master_kvs_addr;

    if (mpi_rank == 0) {
        kvs = env.create_master_kvs();
        master_kvs_addr = kvs->get_addr();
        MPICHECK(
            MPI_Bcast((void*)master_kvs_addr.data(), ccl::master_kvs::addr_max_size, MPI_BYTE, 0, MPI_COMM_WORLD));
    }
    else {
        MPICHECK(
            MPI_Bcast((void*)master_kvs_addr.data(), ccl::master_kvs::addr_max_size, MPI_BYTE, 0, MPI_COMM_WORLD));
        kvs = env.create_kvs(master_kvs_addr);
    }

    /*
        create device communicators
        in this version rank-device mapping will happen automatically
        but user can also provide explicit rank-device mapping
    */
    auto comms =
        ccl::environment::instance().create_device_communicators(mpi_size * devices.size(), devices, kvs);

    /* create SYCL queues and USM buffers */
    std::map<size_t, cl::sycl::queue> rank_queue_map;
    std::map<size_t, ccl::stream_t> rank_stream_map;
    std::map<size_t, std::vector<dtype_ptr>> rank_buf_map;

    for (auto& comm : comms) {

        auto dev = comm->device();
        auto rank = comm->rank();

        /* create SYCL queue and CCL stream */
        cl::sycl::queue sycl_queue(dev);
        rank_queue_map[rank] = sycl_queue;
        rank_stream_map[rank] = env.create_stream(sycl_queue);

        /* allocate send/recv buffers */
        dtype* send_buf = static_cast<dtype*>(
            cl::sycl::aligned_alloc_device(ALIGNMENT, COUNT, sycl_queue));

        dtype* recv_buf = static_cast<dtype*>(
            cl::sycl::aligned_alloc_device(ALIGNMENT, COUNT, sycl_queue));

        if (!send_buf or !recv_buf) {
            std::cerr << "Cannot allocate USM buffers! Abort" << std::endl;
            abort();
        }

        /* fill buffers */
        /* ... */

        rank_buf_map[rank].push_back(send_buf);
        rank_buf_map[rank].push_back(recv_buf);
    }

    /* call allreduce for each local devices/ranks */
    std::vector<ccl::request_t> reqs;
    for (auto& comm : comms) {

        size_t rank = comm->rank();
        auto& buffers = rank_buf_map.find(rank)->second;
        auto& stream = rank_stream_map[rank];

        reqs.push_back(comm->allreduce(buffers[0], buffers[1], COUNT, ccl::reduction::sum, stream));
    }

    /* complete allreduce on all local devices/ranks */
    for (auto& req : reqs) {
        req->wait();
    }

    /* printout */
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);

        for (auto& iter : rank_buf_map) {

            size_t rank = iter.first;
            const auto& buffers = iter.second;
            std::cout << "rank : " << rank << std::endl;

            for (const auto& buf : buffers) {
                std::vector<dtype> tmp(buf, buf + COUNT);
                std::copy(
                    tmp.begin(), tmp.end(), std::ostream_iterator<dtype>(std::cout, ","));
                std::cout << "\n\n" << std::endl;
            }
        }
    }

    MPI_Finalize();

    return 0;
}