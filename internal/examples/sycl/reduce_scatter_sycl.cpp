#include <algorithm>
#include <iostream>
#include <vector>
#include <map>
#include <memory>
#include <string>
#include <thread>

#include <mpi.h>

#include "base.hpp"
#include "internal_utils.hpp"
#include "oneapi/ccl/native_device_api/export_api.hpp"
#include "sycl_base.hpp"

#define COUNT     512
#define COLL_ROOT (0)

template <class processing_type>
void user_thread_idx(size_t thread_idx,
                     const std::vector<std::pair<int, cl::sycl::device>>& devices,
                     cl::sycl::context ctx,
                     int total_devices_in_cluster,
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
        ccl::create_communicators(total_devices_in_cluster, devices, ctx, kvs_instance);

    std::cout << "Create device communicators, expected count: " << devices.size() << std::endl;

    /* alloc memory specific to devices */
    for (auto& comm : comms) {
        int rank = comm.rank();
        /* create device */
        auto sycl_device = comm.get_device().get_native();

        /* create stream */
        queue q(sycl_device);
        streams.emplace(rank, ccl::create_stream(q));

        /* create allocators and put them in a map to keep actual handles of buf_allocator */
        allocators.emplace(
            rank,
            std::unique_ptr<buf_allocator<processing_type>>(new buf_allocator<processing_type>(q)));
        auto& alloc_ptr = allocators.find(rank)->second;

        /* create buffers */
        auto send_buf = alloc_ptr->allocate(COUNT, usm_alloc_type);
        auto recv_buf = alloc_ptr->allocate(COUNT, usm_alloc_type);

        /* open send_buf and modify it on the device side */
        q.submit([&](auto& h) {
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

    /* reduce_scatter */
    std::vector<ccl::event> reqs;
    for (auto& comm : comms) {
        size_t rank = comm.rank();

        /* create operation attributes */
        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto attr = ccl::create_operation_attr<ccl::reduce_scatter_attr>();
        auto& stream = streams.find(rank)->second;

        /* invoke operation */
        reqs.push_back(ccl::reduce_scatter(
            mem_objects[0], mem_objects[1], COUNT, ccl::reduction::sum, comm, stream, attr));
    }

    /* wait */
    for (auto& req : reqs) {
        req.wait();
    }

    /* open recv_buf and check its correctness on the device side */
    buffer<processing_type> check_buf1(COUNT);
    for (auto& comm : comms) {
        int rank = comm.rank();

        /* create operation attributes */
        auto stream = streams.find(rank)->second;
        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        auto recv_buf1 = mem_objects[1];
        auto q = stream.get_native();

        q.submit([&](auto& h) {
            accessor check_buf1_acc(check_buf1, h, write_only);
            h.parallel_for(COUNT / total_devices_in_cluster, [=](auto id) {
                if (recv_buf1[id] != total_devices_in_cluster) {
                    check_buf1_acc[id] = false;
                }

                check_buf1_acc[id] = recv_buf1[id];
            });
        });
    }

    /* print out the result of the test on the host side */
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);

        host_accessor check_buf1_acc(check_buf1, read_only);
        for (int i = 0; i < COUNT / total_devices_in_cluster; i++) {
            if (check_buf1_acc[i] != total_devices_in_cluster) {
                cout << "FAILED at " << i << ", expected: " << total_devices_in_cluster
                     << ", got: " << check_buf1_acc[i] << "\n";
                exit(1);
            }
        }
        cout << "PASSED\n";
    }
}

int main(int argc, char** argv) {
    using namespace ::native;

    //Use:
    // SYCL_BE=PI_OTHER SYCL_PI_TRACE=1 ZE_DEBUG=1  SYCL_DEVICE_WHITE_LIST="" CCL_LOG_LEVEL=info gdb examples/level_zero/l0_thread_allreduce_cpp_test
    // determine GPu device affinity
    /*
     * Affinity mask description:
     *
     *  "0,1,2|3,4,5"     (thread_0 has 0,1,2 devices; thread_1 has 3,4,5)
     *
     *  "0,1,2|3,4,5#6,7,8|9,10,11"    per host group separation
     */
    setenv("L0_CLUSTER_AFFINITY_MASK", "[0:0],[0:0]|[0:0],[0:0]", 0);
    const char* affinity_env_value = getenv("L0_CLUSTER_AFFINITY_MASK");

    /* take usm allocation type, default: shared */
    auto pair_usm_type = take_usm_type(argc, argv[1]);
    std::cout << "USM allocation type: " << pair_usm_type.second << std::endl;
    /* extract GPU affinities by processes using '#' separator from L0_CLUSTER_AFFINITY_MASK */
    std::vector<std::string> process_group_gpu_affinity;
    utils::str_to_array<std::string>(affinity_env_value, process_group_gpu_affinity, '#');

    /* init and register gpu module */
    ccl::init();

    /* get addresses from MPI */
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

    /* build CCL internal KVS */
    auto kvs_instance = utils::build_kvs(mpi_rank);

    /* parse/init data */
    using device_type = ccl::communicator::device_type;

    std::map<size_t, std::vector<device_type>> devices_for_mpi_rank;
    std::map<size_t, ccl::process_device_indices_type> node_device_indices;
    std::map<size_t, std::vector<std::string>> thread_group_gpu_affinity_per_process;
    size_t total_device_in_cluster = 0;
    size_t device_rank_for_mpi_rank_id_offset = 0;

    std::cout << "Expected process count: " << process_group_gpu_affinity.size() << std::endl;
    std::vector<size_t> total_devices_in_process(process_group_gpu_affinity.size(), 0);

    for (size_t process_index = 0; process_index < process_group_gpu_affinity.size();
         process_index++) {
        /* extract  GPU affinities by thread inside process using '|' separator from L0_CLUSTER_AFFINITY_MASK */
        utils::str_to_array<std::string>(process_group_gpu_affinity[process_index].c_str(),
                                         thread_group_gpu_affinity_per_process[process_index],
                                         '|');

        const std::vector<std::string>& thread_gpu_affinity =
            thread_group_gpu_affinity_per_process.find(process_index)->second;

        device_rank_for_mpi_rank_id_offset =
            utils::take_mpi_rank_id_offest(process_index, mpi_size, total_device_in_cluster);

        std::cout << "For process by id: " << process_index
                  << ", expected threads in process count: " << thread_gpu_affinity.size()
                  << std::endl;

        node_device_indices[process_index] =
            utils::extract_indices_for_threads(process_index,
                                               mpi_rank,
                                               thread_gpu_affinity,
                                               total_device_in_cluster,
                                               total_devices_in_process,
                                               devices_for_mpi_rank);
    }

    /* calculate total devices for cluster */
    std::cout << "Devices in cluster count: " << total_device_in_cluster
              << ", for rank: " << mpi_rank << " devices count"
              << total_devices_in_process[mpi_rank] << ", thread count"
              << node_device_indices[mpi_rank].size() << std::endl;

    auto devices_in_process = utils::set_union_devices_in_current_process(devices_for_mpi_rank);

    //TODO: terminate called after throwing an instance of 'cl::sycl::invalid_parameter_error'
    //what():  Can't add devices across platforms to a single context. -33 (CL_INVALID_DEVICE)
    //auto ctx = cl::sycl::context(devices_in_process);
    auto ctx = cl::sycl::context(*devices_in_process.begin()); //use single device

    const auto& thread_group_affinity = devices_for_mpi_rank;
    std::vector<std::thread> thread_group;
    for (auto thread_affinity_it = thread_group_affinity.begin();
         thread_affinity_it != thread_group_affinity.end();
         ++thread_affinity_it) {
        size_t thread_id;
        std::vector<device_type> devices;
        std::tie(thread_id, devices) = *thread_affinity_it;

        std::vector<std::pair<int, device_type>> ranked_devices;
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
                                  pair_usm_type.first);
    }

    /* wait finishing */
    for (auto& t : thread_group) {
        t.join();
    }

    return 0;
}
