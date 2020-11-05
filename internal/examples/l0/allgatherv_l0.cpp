#include <algorithm>
#include <iostream>
#include <stdio.h>
#include <sstream>
#include <vector>
#include <iterator>
#include <map>
#include <memory>
#include <set>
#include <string>
#include <limits>
#include <thread>
#include <numeric>

#include "base.hpp"
#include "base_utils.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"

#define COUNT     512 //(10*1024*1024)
#define COLL_ROOT (0)

int gpu_topology_type = 1; //0 - between devices in single thread
    //1 - between multiple threads in single process
    //2 - between nultiple processes

#ifdef CCL_ENABLE_SYCL
template <class processing_type>
void user_thread_idx(size_t thread_idx,
                     const std::vector<std::pair<size_t, cl::sycl::device>>& devices,
                     cl::sycl::context ctx,
                     int total_devices_in_cluster,
                     std::shared_ptr<ccl::kvs_interface> kvs_instance) {
    using namespace ::native;

    // test data
    using allocated_memory_array = std::vector<processing_type*>;
    using rank_allocated_memory = std::map<size_t, allocated_memory_array>;
    //using native_queue_storage       = std::map<size_t, ccl_device::device_queue>;
    using stream_storage = std::map<size_t, ccl::stream>;
    using device_queue_map = std::map<size_t, cl::sycl::queue>;

    device_queue_map device_queue;
    rank_allocated_memory memory_storage;
    stream_storage streams;
    std::vector<processing_type> send_values(COUNT);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<processing_type> recv_values(COUNT, 0);

    // Create device communicators
    std::vector<ccl::communicator> comms =
        ccl::create_communicators(total_devices_in_cluster, devices, ctx, kvs_instance);

    std::cout << "Create device communicators, expected count: " << devices.size() << std::endl;

    size_t local_topology_size = comms[0].size();
    std::vector<size_t> recv_counts(local_topology_size);
    for (size_t idx = 0; idx < local_topology_size; idx++) {
        recv_counts[idx] = COUNT;
    }

    // alloc memory specific to devices
    for (auto& comm : comms) {
        // get native l0* /
        ccl::communicator::device_type dev = comm.get_device().get_native();
        int rank = comm.rank();

        /* TODO: it is a temporary change. In the previous code,
         * there is ccl::create_stream() seg fault issue. NEED TO FIX THAT.
         */

        // create stream from device communicator directly
        // const cl::sycl::queue& q =
        //     streams.find(rank)->second.get<ccl::stream_attr_id::native_handle>();

        cl::sycl::queue q(dev);
        streams.emplace(rank, ccl::create_stream(q));

        // allocate memory
        processing_type* mem_send = static_cast<processing_type*>(cl::sycl::aligned_alloc_shared(
            alignof(processing_type), COUNT * sizeof(processing_type), q));
        processing_type* mem_recv = static_cast<processing_type*>(cl::sycl::aligned_alloc_shared(
            alignof(processing_type), (local_topology_size * COUNT) * sizeof(processing_type), q));
        // set initial memory
        {
            static std::mutex memory_mutex;

            std::lock_guard<std::mutex> lock(memory_mutex);

            std::iota(mem_send, mem_send + COUNT, 1);
        }

        if (memory_storage[rank].empty()) {
            memory_storage[rank].reserve(100);
        }
        memory_storage[rank].push_back(std::move(mem_send));
        memory_storage[rank].push_back(std::move(mem_recv));
    }

    // allgatherv
    std::vector<ccl::event> reqs;
    for (auto& comm : comms) {
        int rank = comm.rank();
        /*
        if (!comm.is_ready())
        {
            std::cerr << "Communicator by rank: " << rank << " should be ready already" << std::endl;
            abort();
        }*/

        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto& stream = streams.find(rank)->second;

        reqs.push_back(ccl::allgatherv(mem_objects[0], //send_buf
                                       COUNT, //send_count
                                       mem_objects[1], //recv_buf
                                       recv_counts, //recv_counts
                                       comm, //communicator
                                       stream)); //stream
    }
    // end allgatherv

    //wait
    for (auto& req : reqs) {
        req.wait();
    }

    //printout
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);
        for (auto& dev_it : memory_storage) {
            int rank = dev_it.first;
            const auto& handles = dev_it.second;
            std::cout << "rank : " << rank << std::endl;
            for (const auto& mem : handles) {
                // std::vector<processing_type> tmp = mem.enqueue_read_sync();
                std::copy(mem, mem + COUNT, std::ostream_iterator<processing_type>(std::cout, ","));
                std::cout << "\n\n" << std::endl;
            }
        }
    }
}
#else //CCL_ENABLE_SYCL
template <class processing_type>
void user_thread_idx(size_t thread_idx,
                     std::vector<std::pair<size_t, ccl::device_index_type>> ranked_device_indices,
                     std::shared_ptr<::native::ccl_context> ctx,
                     int total_devices_in_cluster,
                     std::shared_ptr<ccl::kvs_interface> kvs) {
    using namespace ::native;

    // test data
    using allocated_memory_array = std::vector<ccl_device::device_memory<processing_type>>;
    using rank_allocated_memory = std::map<size_t, allocated_memory_array>;
    using stream_storage = std::map<size_t, ccl::stream>;

    rank_allocated_memory memory_storage;
    stream_storage streams;
    std::vector<processing_type> send_values(COUNT);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<processing_type> recv_values(COUNT, 0);

    // API
    // Create device communicators
    std::vector<ccl::communicator> comms =
        ccl::create_communicators(total_devices_in_cluster, ranked_device_indices, ctx, kvs);

    std::cout << "Create device communicators, expected count: " << ranked_device_indices.size()
              << ", context: " << ctx.get() << std::endl;

    size_t local_topology_size = comms[0].size();
    std::vector<size_t> recv_counts(local_topology_size);
    for (size_t idx = 0; idx < local_topology_size; idx++) {
        recv_counts[idx] = COUNT;
    }

    // alloc memory specific to devices
    for (auto& comm : comms) {
        // get native l0* /
        ccl::communicator::device_type dev = comm.get_device().get_native();
        int rank = comm.rank();

        // wrapped L0-native API for devices: create native buffers
        auto mem_send = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type), ctx);
        auto mem_recv = dev->alloc_memory<processing_type>(
            COUNT * local_topology_size, sizeof(processing_type), ctx);

        // set initial memory
        {
            static std::mutex memory_mutex;

            std::lock_guard<std::mutex> lock(memory_mutex);

            // wrapped L0-native API for memory: fill device buffers
            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
        }

        if (memory_storage[rank].empty()) {
            memory_storage[rank].reserve(100);
        }
        memory_storage[rank].push_back(std::move(mem_send));
        memory_storage[rank].push_back(std::move(mem_recv));

        // create native stream
        auto str = std::make_shared<native::ccl_device::device_queue>(dev->create_cmd_queue(ctx));
        streams.emplace(rank, ccl::create_stream(str));
    }

    // allgatherv
    std::vector<ccl::event> reqs;
    for (auto& comm : comms) {
        int rank = comm.rank();
        /*
        if (!comm.is_ready())
        {
            std::cerr << "Communicator by rank: " << rank << " should be ready already" << std::endl;
            abort();
        }*/

        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto& stream = streams.find(rank)->second;

        reqs.push_back(ccl::allgatherv(mem_objects[0].get(), //send_buf
                                       mem_objects[0].count(), //send_count
                                       mem_objects[1].get(), //recv_buf
                                       recv_counts, //recv_counts
                                       comm, //communicator
                                       stream)); //stream
    }
    // end allgatherv

    //wait
    for (auto& req : reqs) {
        req.wait();
    }

    //printout
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);
        for (auto& dev_it : memory_storage) {
            int rank = dev_it.first;
            const auto& handles = dev_it.second;
            std::cout << "rank : " << rank << std::endl;
            for (const auto& mem : handles) {
                std::vector<processing_type> tmp = mem.enqueue_read_sync();
                std::copy(
                    tmp.begin(), tmp.end(), std::ostream_iterator<processing_type>(std::cout, ","));
                std::cout << "\n\n" << std::endl;
            }
        }
    }
}
#endif

int main(int argc, char** argv) {
    using namespace ::native;
    setenv("L0_CLUSTER_AFFINITY_MASK", "[0:0],[0:0]|[0:0],[0:0]", 0);
    const char* affinity_env_value = getenv("L0_CLUSTER_AFFINITY_MASK");

    if (argc == 2) {
        std::cout << "Not supported at now, only 'process_ring' built" << std::endl;
    }

    //Use:
    // SYCL_BE=PI_OTHER SYCL_PI_TRACE=1 ZE_DEBUG=1  SYCL_DEVICE_WHITE_LIST="" CCL_LOG_LEVEL=1 gdb examples/level_zero/l0_thread_allreduce_cpp_test

    // determine GPu device affinity
    /*
     * Affinity mask description:
     *
     *  "0,1,2|3,4,5"     (thread_0 has 0,1,2 devices; thread_1 has 3,4,5)
     *
     *  "0,1,2|3,4,5#6,7,8|9,10,11"    per host group separation
     */
    std::vector<std::thread> thread_group;
    std::vector<std::string> process_group_gpu_affinity;
    std::map<size_t, std::vector<std::string>> thread_group_gpu_affinity_per_process;

    using thread_device_indices_t = ccl::process_device_indices_type;
    std::map<size_t, thread_device_indices_t> node_device_indices;

    // extract GPU affinities by processes using '#' separator from L0_CLUSTER_AFFINITY_MASK
    utils::str_to_array<std::string>(affinity_env_value, process_group_gpu_affinity, '#');

    // init and register gpu module
    ccl::init();

    // get addresses from MPI
    int mpi_rank = 0, mpi_size = 0;
    MPI_Init(&argc, &argv);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi_rank);
    MPI_Comm_size(MPI_COMM_WORLD, &mpi_size);
    if (static_cast<size_t>(mpi_size) != process_group_gpu_affinity.size()) {
        std::cerr << "L0_CLUSTER_AFFINITY_MASK is configured for processes: "
                  << process_group_gpu_affinity.size()
                  << ", but MPI requested world size: " << mpi_size << ". Both should to be equal"
                  << std::endl;
        return -1;
    }

    std::cout << "MPI process rank: " << mpi_rank << ", size: " << mpi_size << std::endl;

    // build CCL internal KVS
    std::shared_ptr<ccl::kvs> kvs_instance;
    ccl::kvs::address_type main_addr;
    if (mpi_rank == 0) {
        kvs_instance = ccl::create_main_kvs();
        main_addr = kvs_instance->get_address();

        std::cout << "Master KVS  hast build on addr: " /*<< main_addr*/ << std::endl;
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs_instance = ccl::create_kvs(main_addr);

        std::cout << "Slave KVS hast connected on addr: " /* << main_addr*/ << std::endl;
    }

    size_t total_device_in_cluster = 0;
    std::cout << "Expected process count: " << process_group_gpu_affinity.size() << std::endl;
    std::vector<size_t> total_devices_in_process(process_group_gpu_affinity.size(), 0);

#ifdef CCL_ENABLE_SYCL
    // using cl::sycl::device
    using device_type = ccl::communicator::device_type;
#else
    // using ccl device index
    using device_type = ccl::device_index_type;
#endif
    std::map<size_t, std::vector<device_type>> devices_for_mpi_rank;

    size_t device_rank_for_mpi_rank_id_offset = 0;
    for (size_t process_index = 0; process_index < process_group_gpu_affinity.size();
         process_index++) {
        // extract  GPU affinities by thread inside process using '|' separator from L0_CLUSTER_AFFINITY_MASK
        utils::str_to_array<std::string>(process_group_gpu_affinity[process_index].c_str(),
                                         thread_group_gpu_affinity_per_process[process_index],
                                         '|');

        const std::vector<std::string>& thread_gpu_affinity =
            thread_group_gpu_affinity_per_process.find(process_index)->second;
        thread_device_indices_t thread_group_affinity;

        if (process_index == static_cast<size_t>(mpi_rank)) {
            device_rank_for_mpi_rank_id_offset = total_device_in_cluster;
        }

        std::cout << "For process by id: " << process_index
                  << ", expected threads in process count: " << thread_gpu_affinity.size()
                  << std::endl;
        for (size_t thread_index = 0; thread_index < thread_gpu_affinity.size(); thread_index++) {
            ccl::device_indices_type device_group_affinity;
            utils::str_to_mset<ccl::device_index_type>(
                thread_gpu_affinity[thread_index].c_str(), device_group_affinity, ',');

            std::cout << " Extracted GPU indices for thread by id: " << thread_index
                      << ", devices in threads count: " << device_group_affinity.size()
                      << std::endl;
            total_device_in_cluster += device_group_affinity.size();
            total_devices_in_process[process_index] += device_group_affinity.size();

            thread_group_affinity[thread_index] = device_group_affinity;

            if (process_index == static_cast<size_t>(mpi_rank)) {
                for (auto device_vendor_id : device_group_affinity) {
                    devices_for_mpi_rank[thread_index].push_back(
#ifdef CCL_ENABLE_SYCL
                        ccl::create_from_index(device_vendor_id).device);
#else
                        device_vendor_id);
#endif
                }
            }
        }

        node_device_indices[process_index] = thread_group_affinity;
    }

    // calculate total devices for cluster
    std::cout << "Devices in cluster count: " << total_device_in_cluster
              << ", for rank: " << mpi_rank << " devices count"
              << total_devices_in_process[mpi_rank] << ", thread count"
              << node_device_indices[mpi_rank].size() << std::endl;

    // launch user threads
    const auto& thread_group_affinity = devices_for_mpi_rank;
    std::vector<device_type> devices_in_process;
    for (auto& thread_devices : devices_for_mpi_rank) {
        devices_in_process.insert(
            devices_in_process.end(), thread_devices.second.begin(), thread_devices.second.end());
    }
#ifdef CCL_ENABLE_SYCL
    //TODO: terminate called after throwing an instance of 'cl::sycl::invalid_parameter_error'
    //what():  Can't add devices across platforms to a single context. -33 (CL_INVALID_DEVICE)
    //auto ctx = cl::sycl::context(devices_in_process);
    auto ctx = cl::sycl::context(*devices_in_process.begin()); //use single device
#else
    const auto& drivers = native::get_platform().get_drivers();
    if (drivers.empty()) {
        std::cerr << "No drivers in L0 native paltform. Exit" << std::endl;
        return -1;
    }

    // get GPU driver, it's only one driver at the moment
    auto gpu_driver = drivers.begin()->second;
    decltype(gpu_driver->create_context()) ctx;
    /*default context*/ // = gpu_driver->create_context();
#endif
    for (auto thread_affinity_it = thread_group_affinity.begin();
         thread_affinity_it != thread_group_affinity.end();
         ++thread_affinity_it) {
        size_t thread_id;
        std::vector<device_type> devices;
        std::tie(thread_id, devices) = *thread_affinity_it;

        std::vector<std::pair<size_t, device_type>> ranked_devices;
        ranked_devices.reserve(devices.size());
        std::transform(devices.begin(),
                       devices.end(),
                       std::back_inserter(ranked_devices),
                       [&device_rank_for_mpi_rank_id_offset](const device_type& dev) {
                           return std::make_pair(device_rank_for_mpi_rank_id_offset++, dev);
                       });

        std::cout << "Launch thread: " << thread_id
                  << " with expected local thread device communicators count: " << devices.size()
                  << std::endl;

        thread_group.emplace_back(&user_thread_idx<float>,
                                  thread_id,
                                  ranked_devices,
                                  ctx,
                                  total_device_in_cluster,
                                  kvs_instance);
    }

    //wait finishing
    for (auto& t : thread_group) {
        t.join();
    }

    return 0;
}
