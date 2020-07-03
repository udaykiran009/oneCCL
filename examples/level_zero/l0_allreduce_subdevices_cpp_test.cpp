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
#include "ccl_gpu_modules.h"
#include "native_device_api/export_api.hpp"

#define COUNT     512
#define COLL_ROOT (0)

int gpu_topology_type = 1; //0 - between devices in single thread
    //1 - between multiple threads in single process
    //2 - between nultiple processes

template <typename T>
void str_to_array(const char* input, std::vector<T>& output, char delimiter) {
    if (!input) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.push_back(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}
template <>
void str_to_array(const char* input, std::vector<std::string>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.push_back(processes_input);
    }
}

template <typename T>
void str_to_mset(const char* input, std::multiset<T>& output, char delimiter) {
    if (!input) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.insert(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}

template <>
void str_to_mset(const char* input, std::multiset<ccl::device_index_type>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.insert(ccl::from_string(processes_input));
    }
}

using processing_type = float;
using processing_type_ptr = float*;
#ifdef CCL_ENABLE_SYCL
void user_thread_idx(size_t thread_idx,
                     ccl::device_indices_t thread_device_idx,
                     size_t total_devices_in_process) {
    (void)thread_idx;
    (void)thread_device_idx;
    (void)total_devices_in_process;
}
#else
void user_thread_idx(size_t thread_idx,
                     ccl::device_indices_t thread_device_idx,
                     size_t total_devices_in_process) {
    using namespace ::native;

    // test data
    using allocated_memory_array = std::vector<ccl_device::device_memory<processing_type>>;
    using rank_allocated_memory = std::map<size_t, allocated_memory_array>;
    using native_queue_storage = std::map<size_t, ccl_device::device_queue>;
    using stream_storage = std::map<size_t, ccl::stream_t>;

    rank_allocated_memory memory_storage;
    native_queue_storage queues;
    stream_storage streams;
    std::vector<processing_type> send_values(COUNT);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<processing_type> recv_values(COUNT, 0);

    // API

    // create 'global_communicator' for wire-up processes in cluster
    ccl::shared_communicator_t global_communicator(
        ccl::environment::instance().create_communicator());

    // create 'comm_group' for wire-up threads in processes
    ccl::comm_group_t group = ccl::environment::instance().create_comm_group(
        thread_device_idx.size(), total_devices_in_process, global_communicator);

    // create device communicator attributes
    ccl::device_comm_attr_t my_device_comm_attr = group->create_device_comm_attr();

    // set preferred device topology (OPTIONAL)
    my_device_comm_attr->set_value<ccl_device_preferred_topology_class>(
        ccl::device_topology_type::ring);
    my_device_comm_attr->set_value<ccl_device_preferred_group>(
        ccl::device_group_split_type::process);
    std::cout << "Platform info: " << group->get_context().to_string() << std::endl;
    std::cout << "Create device communicators, expected count: " << thread_device_idx.size()
              << ", preferred topology class: "
              << my_device_comm_attr->get_value<ccl_device_preferred_topology_class>() << std::endl;

    // Create communicators (auto rank balancing, based on ids): range based API
    std::vector<ccl::communicator_t> comms = group->create_communicators(
        thread_device_idx.begin(), thread_device_idx.end(), my_device_comm_attr);

    // alloc memory specific to devices
    for (auto& comm : comms) {
        // get native l0* /
        ccl::communicator::device_native_reference_t dev = comm->get_device();
        size_t rank = comm->rank();

        if (!dev->is_subdevice()) {
            std::cout << "Invalid device type for communicator: " << rank
                      << ". Should be subdevice, got: " << dev->to_string() << std::endl;
            abort();
        }

        // create native buffers
        auto mem_send = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));
        auto mem_recv = dev->alloc_memory<processing_type>(COUNT, sizeof(processing_type));

        // set initial memory
        {
            static std::mutex memory_mutex;

            std::lock_guard<std::mutex> lock(memory_mutex);
            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
        }

        if (memory_storage[rank].empty()) {
            memory_storage[rank].reserve(100);
        }
        memory_storage[rank].push_back(std::move(mem_send));
        memory_storage[rank].push_back(std::move(mem_recv));

        // create native stream
        enum { INSERTED_ITER, RESULT };
        auto queue_it = std::get<INSERTED_ITER>(queues.emplace(rank, dev->create_cmd_queue()));
        streams.emplace(rank, ccl::environment::instance().create_stream(queue_it->second.get()));
    }

    global_communicator->barrier();

    //allreduce
    std::vector<std::shared_ptr<ccl::request>> reqs;
    ccl::coll_attr coll_attr{};
    for (auto& comm : comms) {
        size_t rank = comm->rank();

        if (!comm->is_ready()) {
            std::cerr << "Communicator by rank: " << rank << " should be ready already"
                      << std::endl;
            abort();
        }

        allocated_memory_array& mem_objects = memory_storage.find(rank)->second;
        reqs.push_back(comm->allreduce(mem_objects[0].get(),
                                       mem_objects[1].get(),
                                       mem_objects[1].count(),
                                       ccl::reduction::sum,
                                       &coll_attr,
                                       streams[rank]));
    }

    //wait
    for (auto& req : reqs) {
        req->wait();
    }

    //gpu_comm->barrier(stream);
    //printout
    static std::mutex printout_mutex;
    {
        std::unique_lock<std::mutex> lock(printout_mutex);
        for (auto& dev_it : memory_storage) {
            size_t rank = dev_it.first;
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
    setenv("CreateMultipleRootDevices", "4", 1);
    setenv("CreateMultipleSubDevices", "4", 1);
    setenv("L0_CLUSTER_AFFINITY_MASK", "[0:0:0],[0:0:1]|[0:0:0],[0:0:1]", 0);
    const char* affinity_env_value = getenv("L0_CLUSTER_AFFINITY_MASK");

    if (argc == 2) {
        std::cout << "Not supported at now, only 'process_ring' built" << std::endl;
        gpu_topology_type = std::atoi(argv[1]);
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

    str_to_array<std::string>(affinity_env_value, process_group_gpu_affinity, '#');

    std::cout << "Expected process count: " << process_group_gpu_affinity.size() << std::endl;
    for (size_t process_index = 0; process_index < process_group_gpu_affinity.size();
         process_index++) {
        str_to_array<std::string>(process_group_gpu_affinity[process_index].c_str(),
                                  thread_group_gpu_affinity_per_process[process_index],
                                  '|');
    }

    ccl::communicator_t global_comm = ccl::environment::instance().create_communicator();
    size_t instance_rank = global_comm->rank();
    size_t instance_count = global_comm->size();
    std::cout << "Process rank: " << instance_rank << ", size: " << instance_count << std::endl;

    if (instance_rank >= process_group_gpu_affinity.size()) {
        std::cerr << "L0_CLUSTER_AFFINITY_MASK is too short, required process rank: "
                  << instance_rank << std::endl;
        return -1;
    }
    const std::vector<std::string>& thread_gpu_affinity =
        thread_group_gpu_affinity_per_process.find(instance_rank)->second;
    std::cout << "Users Thread Count: " << thread_gpu_affinity.size() << std::endl;

    // get thread device affinity
    ccl::process_device_indices_t thread_group_affinity;
    size_t total_devices_in_process = 0;
    for (size_t thread_index = 0; thread_index < thread_gpu_affinity.size(); thread_index++) {
        ccl::device_indices_t device_group_affinity;
        str_to_mset<ccl::device_index_type>(
            thread_gpu_affinity[thread_index].c_str(), device_group_affinity, ',');

        total_devices_in_process += device_group_affinity.size();
        thread_group_affinity[thread_index] = device_group_affinity;
    }

    // Register algorithm from kernel source
    register_allreduce_gpu_module_source("kernels/ring_allreduce.spv",
                                         ccl_topology_class_t::ring_algo_class);
    register_allreduce_gpu_module_source("kernels/a2a_allreduce.spv",
                                         ccl_topology_class_t::a2a_algo_class);

    // launch threads
    for (auto thread_affinity_it = thread_group_affinity.begin();
         thread_affinity_it != thread_group_affinity.end();
         ++thread_affinity_it) {
        size_t thread_id;
        ccl::device_indices_t devices;
        std::tie(thread_id, devices) = *thread_affinity_it;

        std::cout << "Launch thread: " << thread_id
                  << " with expected device communicators count: " << devices.size() << std::endl;
        thread_group.emplace_back(&user_thread_idx, thread_id, devices, total_devices_in_process);
    }

    //wait finishing
    for (auto& t : thread_group) {
        t.join();
    }

    return 0;
}
