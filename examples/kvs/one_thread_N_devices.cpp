
#include "base.hpp"

#define COUNT 512 //(10*1024*1024)
#define COLL_ROOT (0)

using processing_type = float;
using processing_type_ptr = float*;

#ifdef CCL_ENABLE_SYCL
void run_test(const size_t size,
              const std::vector<size_t> device_ids,
              std::shared_ptr<ccl::ccl_kvs_interface> kvs)
{
    using namespace ::native;

    // test data storages
    std::map<size_t, cl::sycl::queue>                                   device_queue_map;
    std::map<size_t, ccl::stream_t>                                     rank_stream_map;
    std::map<size_t, std::vector<processing_type_ptr>>                  memory_storage;

    std::vector<ccl::device_communicator_t> comms = ccl::environment::instance().create_device_communicators(size, device_ids, kvs);

    // alloc memory specific to devices
    for(auto &comm : comms)
    {
        ccl::communicator::device_native_reference_t dev = comm->get_device();
        size_t rank = comm->rank();

        // create stream
        cl::sycl::queue q(dev);
        device_queue_map[rank] = q;
        rank_stream_map[rank] = ccl::environment::instance().create_stream(q);

        // allocate memory
        processing_type* mem_send = static_cast<processing_type*>(cl::sycl::aligned_alloc_device(sizeof(processing_type), COUNT, q));
        processing_type* mem_recv = static_cast<processing_type*>(cl::sycl::aligned_alloc_device(sizeof(processing_type), COUNT, q));

        if (!mem_send or !mem_recv)
        {
            std::cerr << "Cannot allocate USM! Abort" << std::endl;
            abort();
        }

        // set initial memory
        std::iota(mem_send, mem_send + COUNT, 1);
        std::fill(mem_recv, mem_recv + COUNT, 0);
        if(memory_storage[rank].empty())
        {
            memory_storage[rank].reserve(2);
        }
        memory_storage[rank].push_back(mem_send);
        memory_storage[rank].push_back(mem_recv);
    }

    //allreduce
    ccl::coll_attr coll_attr{};
    std::vector<std::shared_ptr<ccl::request>> reqs;
    for(auto &comm : comms)
    {
        size_t rank = comm->rank();

        auto& mem_objects = memory_storage.find(rank)->second;
        auto& stream = rank_stream_map[rank];
        reqs.push_back(comm->allreduce(mem_objects[0],
                                       mem_objects[1],
                                       COUNT,
                                       ccl::reduction::sum,
                                       &coll_attr,
                                       stream));
    }

    //wait
    for(auto &req : reqs)
    {
        req->wait();
    }

    //gpu_comm->barrier(stream);
    //printout
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);
        for(auto &dev_it : memory_storage)
        {
            size_t rank = dev_it.first;
            const auto& handles = dev_it.second;
            std::cout << "rank : "  << rank << std::endl;
            for(const auto& data : handles)
            {
                std::vector<processing_type> tmp(data, data + COUNT);
                std::copy(tmp.begin(), tmp.end(), std::ostream_iterator<processing_type>(std::cout, ","));
                std::cout << "\n\n" << std::endl;
            }
        }
    }
}
#else
void run_test(const size_t size,
              const std::vector<size_t> device_ids,
              std::shared_ptr<ccl::ccl_kvs_interface> kvs)
{
    using namespace ::native;

    // test data
    using allocated_memory_array     = std::vector<ccl_device::device_memory<processing_type>>;
    using rank_allocated_memory      = std::map<size_t, allocated_memory_array>;
    using native_queue_storage       = std::map<size_t, ccl_device::device_queue>;
    using stream_storage             = std::map<size_t, ccl::stream_t>;

    rank_allocated_memory memory_storage;
    native_queue_storage  queues;
    stream_storage        streams;
    std::vector<processing_type> send_values(COUNT);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<processing_type> recv_values(COUNT, 0);

    std::vector<ccl::device_communicator_t> comms = ccl::environment::instance().create_device_communicators(size, device_ids, kvs);

    // alloc memory specific to devices
    for(auto &comm : comms)
    {
        // get native l0* /
        ccl::communicator::device_native_reference_t dev = comm->get_device();
        size_t rank = comm->rank();

        // wrapped L0-native API for devices: create native buffers
        auto mem_send = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));
        auto mem_recv = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));

        // set initial memory
        {
            static std::mutex memory_mutex;

            std::lock_guard<std::mutex> lock(memory_mutex);

            // wrapped L0-native API for memory: fill device buffers
            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
        }

        if(memory_storage[rank].empty())
        {
            memory_storage[rank].reserve(100);
        }
        memory_storage[rank].push_back(std::move(mem_send));
        memory_storage[rank].push_back(std::move(mem_recv));

        // create native stream
        enum
        {
            INSERTED_ITER,
            RESULT
        };
        auto queue_it = std::get<INSERTED_ITER>(queues.emplace(rank, dev->create_cmd_queue()));
        streams.emplace(rank, ccl::environment::instance().create_stream(queue_it->second.get()));
    }

    global_communicator->barrier();

    //allreduce
    std::vector<std::shared_ptr<ccl::request>> reqs;
    ccl::coll_attr coll_attr{};
    for(auto &comm : comms)
    {
        size_t rank = comm->rank();

        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        reqs.push_back(comm->allreduce(mem_objects[0].get(),
                                       mem_objects[1].get(),
                                       mem_objects[1].count(),
                                       ccl::reduction::sum,
                                       &coll_attr,
                                       streams[rank]));
    }

    //wait
    for(auto &req : reqs)
    {
        req->wait();
    }

    //gpu_comm->barrier(stream);
    //printout
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);
        for(auto &dev_it : memory_storage)
        {
            size_t rank = dev_it.first;
            const auto& handles = dev_it.second;
            std::cout << "rank : "  << rank << std::endl;
            for(const auto& mem : handles)
            {
                std::vector<processing_type> tmp = mem.enqueue_read_sync();
                std::copy(tmp.begin(), tmp.end(), std::ostream_iterator<processing_type>(std::cout, ","));
                std::cout << "\n\n" << std::endl;
            }
        }
    }
}
#endif // CCL_ENABLE_SYCL

int main(int argc, char** argv)
{

    using namespace ::native;

    // TODO: add logic for device_id
    size_t size;
    std::vector<size_t> device_ids;
    int rank, size;


    //initializing MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    std::shared_ptr<ccl::ccl_internal_kvs> kvs;
    ccl::ccl_master_addr master_addr;
    if (rank == 0)
    {
        kvs = std::make_shared<ccl::ccl_internal_kvs>();
        master_addr = kvs->get_master_addr();
        MPICHECK(MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD));
    }
    else
    {
        MPICHECK(MPI_Bcast((void *)master_addr.data(), master_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD));
        kvs = std::make_shared<ccl::ccl_internal_kvs>(master_addr);
    }

    run_test(size * device_ids.size(), device_ids, kvs)

    MPI_Finalize();

    return 0;
}