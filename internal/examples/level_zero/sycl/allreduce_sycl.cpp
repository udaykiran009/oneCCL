    #include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <mpi.h>

#include "base.hpp"
#include "base_utils.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "sycl_base.hpp"

#define COUNT     512
#define COLL_ROOT (0)

template <class processing_type>
void user_thread_idx(size_t thread_idx,
                     const std::vector<std::pair<size_t, cl::sycl::device>>& devices,
                     cl::sycl::context ctx,
                     size_t total_devices_in_cluster,
                     std::shared_ptr<ccl::kvs_interface> kvs_instance,
                     usm::alloc usm_alloc_type) {
    using namespace ::native;
    using namespace std;
    using namespace sycl;

    using allocated_memory_array = std::vector<processing_type*>;
    using rank_allocated_memory = std::map<size_t, allocated_memory_array>;
    using stream_storage = std::map<size_t, ccl::stream>;
    using allocator_map_storage = std::map<size_t, std::unique_ptr<buf_allocator<processing_type>>>;

    rank_allocated_memory memory_storage;
    stream_storage streams;
    allocator_map_storage allocators;

    /* create device communicators */
    std::vector<ccl::communicator> comms =
        ccl::create_communicators(
             total_devices_in_cluster, devices, ctx, kvs_instance);

    std::cout << "Create device communicators, expected count: " << devices.size() << std::endl;

    /* alloc memory specific to devices */
    for (auto& comm : comms) {
        size_t rank = comm.rank();
        /* create device */
        auto sycl_device = comm.get_device().get_native();

        /* create stream */
        queue q(sycl_device);
        streams.emplace(rank, ccl::create_stream(q));

        /* create allocators and put them in a map to keep actual handles of buf_allocator */
        allocators.emplace(rank, std::unique_ptr<buf_allocator<processing_type>>
                          (new buf_allocator<processing_type>(q)));
        auto& alloc_ptr = allocators.find(rank)->second;

        /* create buffers */
        // auto usm_alloc_type = usm::alloc::shared;
        auto send_buf = alloc_ptr->allocate(COUNT, usm_alloc_type);
        auto recv_buf = alloc_ptr->allocate(COUNT, usm_alloc_type);

        /* open send_buf and modify it on the device side */
        q.submit([&](auto &h) {
            h.parallel_for(COUNT, [=](auto id) {
                send_buf[id] = 1;
                recv_buf[id] = -1;
            });
        });

        if (memory_storage[rank].empty()) {
            memory_storage[rank].reserve(100);
        }
        memory_storage[rank].push_back(std::move(send_buf));
        memory_storage[rank].push_back(std::move(recv_buf));
    }

    /* allreduce */
    std::vector<ccl::event> reqs;
    for (auto& comm : comms) {
        size_t rank = comm.rank();

        /* create operation attributes */
        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto attr = ccl::create_operation_attr<ccl::allreduce_attr>();
        auto& stream = streams.find(rank)->second;

        /* invoke operation */
        reqs.push_back(ccl::allreduce(mem_objects[0],
                                      mem_objects[1],
                                      COUNT,
                                      ccl::reduction::sum,
                                      comm,
                                      stream,
                                      attr));
    }

    /* wait */
    for (auto& req : reqs) {
        req.wait();
    }

    /* open recv_buf and check its correctness on the device side */
    buffer<processing_type> check_buf(COUNT);
    for (auto& comm : comms) {
        size_t rank = comm.rank();

        /* create operation attributes */
        auto stream = streams.find(rank)->second;
        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto recv_buf = mem_objects[1];
        auto q = stream.get_native();

        q.submit([&](auto &h) {
            accessor check_buf_acc(check_buf, h, write_only);
            h.parallel_for(COUNT, [=](auto id) {
                if (recv_buf[id] != 1 * total_devices_in_cluster) {
                    check_buf_acc[id] = -1;
                }
            });
        });
    }

    /* print out the result of the test on the host side */
    {
        int i;
        host_accessor check_buf_acc(check_buf, read_only);
        for (i = 0; i < COUNT; i++) {
            if (check_buf_acc[i] == -1) {
                cout << "FAILED\n";
                break;
            }
        }
        if (i == COUNT) {
            cout << "PASSED\n";
        }
    }
}

int main(int argc, char** argv) {
    using namespace ::native;
    setenv("L0_CLUSTER_AFFINITY_MASK", "[0:0],[0:0]|[0:0],[0:0]", 0);
    const char* affinity_env_value = getenv("L0_CLUSTER_AFFINITY_MASK");

    auto usm_alloc_type = usm::alloc::shared;
    auto str_usm_alloc_type = "shared";
    if (argc > 1) {
        str_usm_alloc_type = argv[1];
        usm_alloc_type = usm_alloc_type_from_string(argv[1]);
    }
    std::cout << "USM allocation type: " << str_usm_alloc_type << std::endl;

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

    using thread_device_indices_t = ccl::process_device_indices_t;
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

    using device_type = ccl::communicator::ccl_device_t;

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
            ccl::device_indices_t device_group_affinity;
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
                        ccl::create_from_index(device_vendor_id).device);

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

    //TODO: terminate called after throwing an instance of 'cl::sycl::invalid_parameter_error'
    //what():  Can't add devices across platforms to a single context. -33 (CL_INVALID_DEVICE)
    //auto ctx = cl::sycl::context(devices_in_process);
    auto ctx = cl::sycl::context(*devices_in_process.begin()); //use single device

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
                                  kvs_instance,
                                  usm_alloc_type);
    }

    //wait finishing
    for (auto& t : thread_group) {
        t.join();
    }

    return 0;
}
