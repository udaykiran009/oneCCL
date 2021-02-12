#pragma once

#include <memory>
#include <sstream>

#include "allreduce_fixture.hpp"
#include "multi_tile_utils.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_server.hpp"
#include "common/comm/l0/devices/communication_structs/ipc_connection.hpp"

DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, sum);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, prod);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, min);
DEFINE_KERNEL_TYPES_FOR_OP_BF16(allreduce, max);

namespace ring_single_device_multi_tile_ipc_case {

TYPED_TEST_CASE(ring_allreduce_multi_process_fixture, TestTypesAndOpsReduction);
TYPED_TEST(ring_allreduce_multi_process_fixture, ring_single_device_multi_tile_ipc) {
    using namespace native;
    using namespace net;

    // Type of our test
    using native_type = typename TypeParam::first_type;
    using op_type = typename TypeParam::second_type;

    // test case data
    const size_t buffer_size = 512;
    constexpr size_t comm_group_count = 3;
    constexpr size_t mem_group_count = 3;
    constexpr size_t flag_group_count = 3;
    constexpr size_t ipc_mem_group_count = 1;
    constexpr size_t ipc_flag_group_count = 2;

    // check test preconditions
    const auto& devices = this->get_local_devices();
    const auto& global_devices = this->get_global_devices();
    UT_ASSERT(devices.size() >= 1,
              "Devices count: " << devices.size()
                                << " should be greater or equal than 1 for multitile ipc case");

    UT_ASSERT(global_devices.size() >= devices.size() * 2,
              "Global Devices count: " << global_devices.size()
                                       << " should be double from local devices count: "
                                       << devices.size());

    UT_ASSERT(devices.size() == this->get_unique_local_devices().size(),
              "Devices must be unique to launch multi device case");

    // establish relative processes connections (TODO Move to fixture)
    ipc_server server;
    std::string addr{ "ipc_multi_tile_suite_" };
    std::string my_addr = addr + std::to_string(*this->my_pid);
    std::string other_addr = addr + std::to_string(*this->other_pid);

    // start server
    this->output << "PID: " << *this->my_pid << " start server: " << my_addr << std::endl;
    server.start(my_addr);

    unsigned char phase = 0;
    this->wait_phase(phase++);
    std::unique_ptr<ipc_tx_connection> tx_conn(new ipc_tx_connection(other_addr));

    // wait incoming connection
    this->wait_phase(phase++);

    std::unique_ptr<ipc_rx_connection> rx_conn = server.process_connection();
    UT_ASSERT(rx_conn, "RX connection have to be accepted at moment");

    // START TEST execution
    // fill device memory stencil data
    std::shared_ptr<ccl_context> ctx;
    std::vector<native_type> send_values(buffer_size);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<native_type> recv_values(buffer_size, 0);

    // allocate & initialize test kernel params
    int rank_device_idx = 0;
    int rank_size = 0;
    const auto total_dev_indices = this->get_cluster_platform_device_indices();
    for (const auto& process_indices : total_dev_indices) {
        const ccl::process_device_indices_type& thread_indices = process_indices.second;
        for (const auto& thread_index : thread_indices) {
            rank_size += thread_index.second.size();
        }
    }

    size_t device_index_start_offset =
        this->is_child() * devices.size(); /* global cluster numeration */
    this->output << "PID: " << this->pid << " calculated world size: " << rank_size
                 << ", device_index_start_offset: " << device_index_start_offset << std::endl;
    for (const std::shared_ptr<ccl_device>& device : devices) {
        try {
            // initialize communication params
            int rank_idx = rank_device_idx + device_index_start_offset;
            size_t elem_count = buffer_size;

            this->register_shared_comm_data(rank_device_idx, rank_idx, rank_size, elem_count);

            // allocate flags & memory
            // memory
            auto mem_send =
                device->alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto mem_recv =
                device->alloc_memory<native_type>(buffer_size, sizeof(native_type), ctx);
            auto temp_recv = device->alloc_memory<native_type>(
                buffer_size / devices.size(), sizeof(native_type), ctx);
            mem_send.enqueue_write_sync(send_values);
            mem_recv.enqueue_write_sync(recv_values);
            temp_recv.enqueue_write_sync(recv_values.begin(),
                                         recv_values.begin() + buffer_size / devices.size());

            this->register_ipc_memories_data(ctx, rank_device_idx, &temp_recv);
            //ipc_mem_storage_to_send[rank_idx].push_back(device_ptr->create_ipc_memory_handle(temp_recv.handle, ctx));

            /* fill array in specific order
             * l - left
             * r - right
             * Left: l_send, l_recv, l_tmp_recv, r_tmp_recv
             * Right: r_send, r_recv, r_tmp_recv, l_tmp_recv
             */
            this->register_shared_memories_data(
                rank_device_idx, std::move(mem_send), std::move(mem_recv), std::move(temp_recv));

            // flags
            auto left_wrote_2_me_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            auto read_for_receive_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            auto barrier_flag = device->alloc_memory<int>(1, sizeof(int), ctx);
            left_wrote_2_me_flag.enqueue_write_sync({ (int)0 });
            read_for_receive_flag.enqueue_write_sync({ (int)0 });
            barrier_flag.enqueue_write_sync({ (int)0 });

            this->register_ipc_flags_data(
                ctx, rank_device_idx, &left_wrote_2_me_flag, &read_for_receive_flag);

            /* fill array in specific order
             * Left: l_L, l_R, l_B, r_L, r_R
             * Right: r_L, r_R, r_B, l_L, L_R
             */
            this->register_shared_flags_data(rank_device_idx,
                                             std::move(left_wrote_2_me_flag),
                                             std::move(read_for_receive_flag),
                                             std::move(barrier_flag));
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot allocate memory for device num: %" << rank_device_idx
                                                                 << "\nError: " << ex.what());
        }

        rank_device_idx++;
    }

    this->finalize_data_registration(comm_group_count, mem_group_count, flag_group_count);

    // create ipc handle from memory & flags handles
    this->output << "PID: " << *this->my_pid << " create ipc handle from memory handles"
                 << std::endl;
    using ipc_handle_container = std::vector<native::ccl_device::device_ipc_memory_handle>;
    using ipc_handles_ptr_container =
        std::vector<std::shared_ptr<ccl_device::device_ipc_memory_handle>>;

    std::map<ccl_device*, ipc_handle_container> ipc_mem_storage_to_send;
    std::map<ccl_device*, ipc_handles_ptr_container> ipc_mem_storage_to_receive;

    std::map<ccl_device*, ipc_handle_container> ipc_flags_storage_to_send;
    std::map<ccl_device*, ipc_handles_ptr_container> ipc_flags_storage_to_receive;
    size_t thread_id = 0;
    for (auto device : devices) {
        ccl_device* device_ptr = device.get();

        // collect IPC memory
        try {
            auto& memory_handles = this->get_ipc_memory_storage().get_handles_container(thread_id);

            // prepare send handles and receive handles storage
            ipc_handle_container& out_ipc_handles = ipc_mem_storage_to_send[device_ptr];
            out_ipc_handles.reserve(memory_handles.size());

            ipc_handles_ptr_container& in_ipc_handles = ipc_mem_storage_to_receive[device_ptr];
            in_ipc_handles.reserve(memory_handles.size());

            // create ic handles to send it later
            std::transform(memory_handles.begin(),
                           memory_handles.end(),
                           std::back_inserter(out_ipc_handles),
                           [](std::shared_ptr<native::ccl_device::device_ipc_memory_handle>&
                                  memory_handle_ptr) {
                               return std::move(*memory_handle_ptr);
                           });
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot create IPC memory handle for device: " << device_ptr->to_string()
                                                                     << "\nError: " << ex.what());
        }

        // collect IPC flags
        try {
            auto& flags_handles = this->get_ipc_flags_storage().get_handles_container(thread_id);

            // prepare send handles and receive handles storage
            ipc_handle_container& out_ipc_handles = ipc_flags_storage_to_send[device_ptr];
            out_ipc_handles.reserve(flags_handles.size());

            ipc_handles_ptr_container& in_ipc_handles = ipc_flags_storage_to_receive[device_ptr];
            in_ipc_handles.reserve(flags_handles.size());

            // create ic handles to send it later
            std::transform(
                flags_handles.begin(),
                flags_handles.end(),
                std::back_inserter(out_ipc_handles),
                [](std::shared_ptr<native::ccl_device::device_ipc_memory_handle>& flag_handle_ptr) {
                    return std::move(*flag_handle_ptr);
                });
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false,
                      "Cannot create IPC flags handle for device: " << device_ptr->to_string()
                                                                    << "\nError: " << ex.what());
        }
        thread_id++;
    }

    // send to relative process
    // send MEM data
    std::map<ccl_device*, std::vector<uint8_t>> received_raw_data_per_device;

    for (const auto& device_storage : ipc_mem_storage_to_send) {
        const ipc_handle_container& ipc_mem_handles = device_storage.second;
        std::vector<uint8_t> serialized_raw_handles =
            tx_conn->send_ipc_memory(ipc_mem_handles, *this->my_pid);

        //prepare recv array
        received_raw_data_per_device[device_storage.first].resize(serialized_raw_handles.size());
    }

    // receive MEM data
    using ipc_client_shared = std::shared_ptr<native::ccl_device::device_ipc_memory>;
    using ipc_client_handles_container = std::list<ipc_client_shared>;

    std::map<size_t, ipc_client_handles_container> ipc_client_memory;

    thread_id = 0;
    for (auto& device_storage : ipc_mem_storage_to_receive) {
        ipc_handles_ptr_container& ipc_handles = device_storage.second;
        std::vector<uint8_t>& data_to_recv = received_raw_data_per_device[device_storage.first];

        size_t received_rank = 0;
        auto got = rx_conn->receive_ipc_memory(data_to_recv, received_rank);
        ipc_handles.swap(got);

        UT_ASSERT(static_cast<int>(received_rank) == *this->other_pid, "Received rank is invalid");

        // recover handles
        size_t index = 0;
        for (auto& recv_ipc_handle : ipc_handles) {
            std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
            auto ipc_device_it =
                std::find_if(global_devices.begin(),
                             global_devices.end(),
                             [owner_device](const std::shared_ptr<ccl_device>& dev) {
                                 return dev->handle == owner_device->handle;
                             });
            UT_ASSERT(ipc_device_it != global_devices.end(),
                      "Cannot find ipc device in global driver");

            this->output << "PID: " << *this->my_pid << " restore MEM from IPC handle" << std::endl;
            try {
                std::shared_ptr<ccl_device::device_ipc_memory> recovered_mem =
                    owner_device->restore_shared_ipc_memory(std::move(recv_ipc_handle), ctx);

                ipc_client_memory[thread_id].push_back(recovered_mem);
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Cannot restore ipc MEM handle at index: " << index
                                                                     << ". Error: " << ex.what());
            }
            index++;
        }

        thread_id++;
    }

    // send Flag data
    for (const auto& device_storage : ipc_flags_storage_to_send) {
        const ipc_handle_container& ipc_flag_handles = device_storage.second;
        std::vector<uint8_t> serialized_raw_handles =
            tx_conn->send_ipc_memory(ipc_flag_handles, *this->my_pid);

        //prepare recv array
        received_raw_data_per_device[device_storage.first].clear();
        received_raw_data_per_device[device_storage.first].resize(serialized_raw_handles.size());
    }

    // receive FLAG data
    std::map<size_t, ipc_client_handles_container> ipc_client_flags;

    thread_id = 0;
    for (auto& device_storage : ipc_flags_storage_to_receive) {
        ipc_handles_ptr_container& ipc_handles = device_storage.second;
        std::vector<uint8_t>& data_to_recv = received_raw_data_per_device[device_storage.first];

        size_t received_rank = 0;
        auto got = rx_conn->receive_ipc_memory(data_to_recv, received_rank);
        ipc_handles.swap(got);

        UT_ASSERT(static_cast<int>(received_rank) == *this->other_pid, "Received rank is invalid");

        // recover handles
        size_t index = 0;
        for (auto& recv_ipc_handle : ipc_handles) {
            std::shared_ptr<ccl_device> owner_device = recv_ipc_handle->get_owner().lock();
            auto ipc_device_it =
                std::find_if(global_devices.begin(),
                             global_devices.end(),
                             [owner_device](const std::shared_ptr<ccl_device>& dev) {
                                 return dev->handle == owner_device->handle;
                             });
            UT_ASSERT(ipc_device_it != global_devices.end(),
                      "Cannot find ipc device in global driver");

            this->output << "PID: " << *this->my_pid << " restore FLAG from IPC handle"
                         << std::endl;
            try {
                std::shared_ptr<ccl_device::device_ipc_memory> recovered_mem =
                    owner_device->restore_shared_ipc_memory(std::move(recv_ipc_handle), ctx);

                ipc_client_flags[thread_id].push_back(std::move(recovered_mem));
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Cannot restore ipc FLAG handle at index: " << index
                                                                      << ". Error: " << ex.what());
            }
            index++;
        }
        thread_id++;
    }

    // prepare kernels
    for (size_t device_index = 0; device_index < devices.size(); device_index++) {
        this->create_kernel(device_index,
                            allreduce_param_traits<native_type, op_type>::ipc_kernel_name);
    }

    // prepare queues & lists
    this->output << "PID: " << this->pid << "Prepare queue and lists \n";
    std::map<size_t, ccl_device::device_queue> thread_queue;
    std::map<size_t, ccl_device::device_cmd_list> thread_cmd_list;
    multi_tile_utils::prepare_queues_and_lists(devices,
                                               ctx,
                                               thread_queue,
                                               thread_cmd_list,
                                               this->output,
                                               device_index_start_offset /* cluster offset */);

    //printout memory handles
    this->output << "PID: " << this->pid << "\n*************************\n";
    this->dump_memory(this->output, true);
    this->output << std::endl;

    //Set args and launch kernel
    std::vector<std::thread> thread_group;
    std::vector<std::unique_ptr<std::stringstream>> thread_out_put;
    for (const auto& device : devices) {
        (void)device;
        size_t thread_idx = thread_group.size();
        ze_kernel_handle_t kernel = this->get_kernel(thread_idx);
        auto mem_handles = this->get_memory_handles(thread_idx);
        auto flag_handles = this->get_flag_handles(thread_idx);
        auto comm_handles = this->get_comm_handles(thread_idx);

        //ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        ccl_device::device_queue& queue = thread_queue.find(thread_idx)->second;
        //ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;
        ccl_device::device_cmd_list& list = thread_cmd_list.find(thread_idx)->second;

        this->output << "PID: " << this->pid << ", start thread for kernel execution" << std::endl;
        std::unique_ptr<std::stringstream> out_ptr(new std::stringstream());
        std::stringstream* raw_out = out_ptr.get();
        thread_group.emplace_back([this,
                                   thread_idx,
                                   kernel,
                                   &list,
                                   &queue,
                                   mem_handles,
                                   flag_handles,
                                   &ipc_client_memory,
                                   &ipc_client_flags,
                                   comm_handles,
                                   raw_out]() {
            auto& ipc_mem_handles = ipc_client_memory.find(thread_idx)->second;
            auto& ipc_flag_handles = ipc_client_flags.find(thread_idx)->second;

            std::stringstream& out = *raw_out;
            ze_group_count_t launch_args = { 1, 1, 1 };
            try {
                           // set group size
            uint32_t groupSizeX = buffer_size;
            uint32_t groupSizeY = 1u;
            uint32_t groupSizeZ = 1u;
            if(zeKernelSetGroupSize(kernel, groupSizeX, groupSizeY, groupSizeZ != ZE_RESULT_SUCCESS)) {
               throw std::runtime_error(std::string("Cannot set kernel group size, error") );
            }

                // bind rank, size, buffer_size
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", comm_handles: \n"
                    << std::endl;
                std::array<int, 3> comm_offset{ 0, 1, 2 };
                UT_ASSERT(comm_offset.size() == comm_handles.size(), "comm_offset != comm_handles");
                bind_kernel_args(kernel, thread_idx, comm_offset, comm_handles);

                // bind l_send, l_recv, l_tmp,
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", mem_handles: \n";
                std::array<int, mem_group_count> mem_offset{ 3, 4, 5 };
                bind_kernel_args(kernel, thread_idx, mem_offset, mem_handles);

                // Bind IPC memory
                std::array<int, ipc_mem_group_count> ipc_mem_offset{ 9 };
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", ipc_mem_handles: \n";
                bind_kernel_args(kernel, thread_idx, ipc_mem_offset, ipc_mem_handles);

                // bindleft_wrote_2_me_flag, read_for_receive_flag, local_barrier_flag
                std::array<int, flag_group_count> flag_offset{ 6, 7, 8 };
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, thread_idx, flag_offset, flag_handles);

                // Bind IPC flags
                std::array<int, ipc_flag_group_count> ipc_flag_offset{ 10, 11 };
                out << "PID: " << this->pid << ", thread_idx: " << thread_idx
                    << ", ipc_flag_handles: \n"
                    << std::endl;
                bind_kernel_args(kernel, thread_idx, ipc_flag_offset, ipc_flag_handles);

                ze_result_t ret = zeCommandListAppendLaunchKernel(
                    list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                if (ret != ZE_RESULT_SUCCESS) {
                    throw std::runtime_error(
                        std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                        std::to_string(ret));
                }

                //no sync processes here
                {
                    ze_result_t ret = ZE_RESULT_SUCCESS;
                    {
                        ret = zeCommandListAppendLaunchKernel(
                            list.handle, kernel, &launch_args, nullptr, 0, nullptr);
                        if (ret != ZE_RESULT_SUCCESS) {
                            throw std::runtime_error(
                                std::string("cannot zeCommandListAppendLaunchKernel, error: ") +
                                std::to_string(ret));
                        }
                    }
                }

                queue_sync_processing(list, queue);
                out << "thread finished" << std::endl;
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Exception in PID: " << this->pid << ",thread: " << thread_idx
                                               << "\nError: " << ex.what() << ", at phase:\n{\n"
                                               << out.str() << "\n}\n");
                throw;
            }
        });

        thread_out_put.push_back(std::move(out_ptr));
    }

    size_t index = 0;
    this->output << "PID: " << this->pid << ", wating threads" << std::endl;
    for (auto& t : thread_group) {
        //sleep(15);
        try {
            t.join();
            this->output << "PID: " << this->pid << "\n*************************\n"
                         << thread_out_put[index]->str() << "\n*************************\n"
                         << std::endl;
        }
        catch (const std::exception& ex) {
            this->output << "PID: " << this->pid << "\n******ERROR*******************\n"
                         << thread_out_put[index]->str() << "\n*************************\n"
                         << ex.what() << std::endl;
        }
        //t.join();
        index++;
    }

    uint8_t ready = 0;
    if (this->is_child()) {
        utils::readFromSocket(this->communication_socket, &ready, sizeof(ready));
    }
    else {
        ready = 1;
        utils::writeToSocket(this->communication_socket, &ready, sizeof(ready));
    }
    this->output << "PID: " << this->pid << ", finished, status: " << ready << std::endl;
    //printout
    /*
    memory_storage.dump(this->output);
    flags_storage.dump(this->output);
    * */
    quick_exit(0);
}

} // namespace ring_single_device_multi_tile_ipc_case
