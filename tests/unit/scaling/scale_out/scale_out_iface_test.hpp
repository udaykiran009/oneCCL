#pragma once

#include "scale_out_fixture.hpp"

namespace scale_out_iface_test {
// TODO: make a template test (use different data types)
TEST_F(scale_out_fixture, scale_out_iface_test) {
    using namespace native;
    using namespace native::observer;
    using session_key_t = session_key;

    // get driver to create a context
    auto drv_it = local_platform->drivers.find(0);
    UT_ASSERT(drv_it != local_platform->drivers.end(), "Driver by idx 0 must exist!");
    ccl_device_driver& driver = *drv_it->second;

    // get local devices
    const auto& devices = this->get_local_devices();
    UT_ASSERT(devices.size() > 0,
              "SingleDevice test scope require at least 1 device in local platform! Use correct \""
                  << ut_device_affinity_mask_name << "\"");

    size_t comm_size = devices.size();

    this->output << "device id: " << ccl::to_string(devices[0]->get_device_path()) << std::endl;

    // create context based on driver
    std::shared_ptr<ccl_context> ctx = driver.create_context();

    // create a session key for creating a full-fledged session
    const int* key = new int(1);
    const session_key_t sess_key(key);
    size_t elem_count = 1024 * 10; //10 mb
    size_t ring_idx = 0;
    size_t ring_count = 1;

    producer_description producer_descr_params{ .rank = 0,
                                                .comm_size = comm_size,
                                                .staged_buffer_elem_count =
                                                    elem_count, // The size of quanity elements
                                                .context = ctx,
                                                .device = *devices[0],
                                                .immediate_list =
                                                    devices[0]->create_immediate_cmd_list(ctx) };

    // crate numa seesion
    auto numa_sess = numa_session<ccl::device_topology_type::ring,
                                  invoke_params<ccl_coll_bcast, kernel_params_default<int>>>(
        producer_descr_params,
        ring_idx, // index of a ring
        ring_count, // count of a ring
        sess_key);

    // create invoke session params
    invoke_params<ccl_coll_bcast, kernel_params_default<int>> invoke_session_params(
        std::move(producer_descr_params));
    // setting invoke session params
    invoke_session_params.set_out_params(numa_sess.get_ctx_descr());
    // getting invoke session params
    auto ctx_params = invoke_session_params.get_ctx_params();

    // host side
    auto host_mem_buff = ctx_params.host_mem_producer->get(); // host mem buff
    auto host_mem_size = ctx_params.host_mem_producer_counter->get(); // host mem buff size
    ctx_params.host_expected_bytes = elem_count; // host mem buff
    size_t host_mem_buff_size = elem_count;
    void* chunk_host_buff = nullptr;

    for (size_t i = 0; i < host_mem_buff_size; i++) {
        host_mem_buff[i] = i;
    }

    // 10mb / 43 chunks. 43 is just for testing
    // We can handle any of number of chunks(even or odd)
    size_t chunk_counts = 43;
    size_t chunk_host_buff_size = 0;
    size_t offset_idx = 0;
    size_t chunk_idx = 0;
    while (!numa_sess.is_produced()) {
        this->output << "-------------------------------------------------" << std::endl;
        // setting host mem buff size
        if (chunk_idx <= chunk_counts - 1)
            *host_mem_size = (host_mem_buff_size / chunk_counts) * (chunk_idx + 1);

        offset_idx += chunk_host_buff_size;
        numa_sess.produce_data(&chunk_host_buff, chunk_host_buff_size);
        // checking results
        for (size_t i = 0; i < chunk_host_buff_size; i++) {
            UT_ASSERT(((int*)chunk_host_buff)[i] == ((int*)host_mem_buff)[i + offset_idx],
                      "Error: Gotten incorrect results after produce phase: "
                          << "At index " << i << ", gotten: " << ((int*)chunk_host_buff)[i]
                          << ", but expected: " << ((int*)host_mem_buff)[i + offset_idx]);
        }
        this->output << "For chunk: " << chunk_idx + 1
                     << ",checking results from host buffer:   PASSED" << std::endl;

        size_t chunk_dev_buff_size = chunk_host_buff_size;
        void* chunk_dev_buff = malloc(chunk_dev_buff_size * sizeof(int));
        memcpy(chunk_dev_buff, chunk_host_buff, chunk_dev_buff_size * sizeof(int));
        for (size_t i = 0; i < chunk_dev_buff_size; i++) {
            ((int*)chunk_dev_buff)[i] = ((int*)chunk_dev_buff)[i] * 2;
        }

        // device side
        std::vector<int> res_dev_buff;
        std::vector<int> res_dev_buff_size;

        numa_sess.consume_data(0, chunk_dev_buff, chunk_dev_buff_size);
        UT_ASSERT(numa_sess.is_consumed(), "Invoke of consume data: FAILED");

        // get mem buf and  mem buf size from a device
        auto check_dev_mem_buff = ctx_params.dev_mem_consumer->enqueue_read_sync();
        auto check_dev_mem_buff_size = ctx_params.dev_mem_consumer_counter->enqueue_read_sync();

        // migrate data from device buffer to tmp buffers to check results
        // buff
        std::copy(
            check_dev_mem_buff.begin(), check_dev_mem_buff.end(), std::back_inserter(res_dev_buff));

        // size
        std::copy(check_dev_mem_buff_size.begin(),
                  check_dev_mem_buff_size.end(),
                  std::back_inserter(res_dev_buff_size));

        // checking results with i * 2 pattern
        for (int i = offset_idx; i < res_dev_buff_size[0]; i++) {
            UT_ASSERT(res_dev_buff[i] == i * 2,
                      "Error: Gotten inorrect results from device buffer after consume phase: "
                          << "At index " << i << ", gotten: " << res_dev_buff[i]
                          << ", but expected: " << i * 2);
        }
        this->output << "For chunk: " << chunk_idx + 1
                     << ",checking results from device buffer: PASSED" << std::endl;

        if (chunk_idx == chunk_counts - 1 &&
            ctx_params.host_expected_bytes != ctx_params.host_consumed_bytes) {
            *host_mem_size = ctx_params.host_expected_bytes - ctx_params.host_consumed_bytes;
        }
        chunk_idx++;
        free(chunk_dev_buff);

        if (numa_sess.is_produced()) {
            this->output << "Test is passed!" << std::endl;
            break;
        }
    }
}

} // namespace scale_out_iface_test