#pragma once

#include <numeric>

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

template <class DType>
class ring_allgatherv_single_process_fixture : public platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_allgatherv_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~ring_allgatherv_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_allgatherv.spv");
    }

    void TearDown() override {}
};

template <class DType>
class ring_allgatherv_multi_process_fixture : public multi_platform_fixture,
                                              public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_allgatherv_multi_process_fixture() : multi_platform_fixture(get_global_device_indices()) {}

    ~ring_allgatherv_multi_process_fixture() override {}

    void SetUp() override {
        multi_platform_fixture::SetUp();

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_allgatherv.spv");
    }

    void TearDown() override {}
};

template <class DType, class Object>
void alloc_and_fill_allgatherv_buffers(
    Object obj,
    std::vector<size_t> recv_counts,
    std::vector<size_t> recv_offsets,
    const std::vector<native::ccl_device_driver::device_ptr>& devs,
    std::shared_ptr<native::ccl_context> ctx,
    bool with_ipc = false) {
    size_t total_recv_count = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
    std::vector<DType> send_values = get_initial_send_values<DType>(total_recv_count);
    std::vector<DType> recv_values(total_recv_count, 0);

    for (size_t rank = 0; rank < devs.size(); rank++) {
        size_t send_count = recv_counts[rank];
        auto send_buf = devs[rank]->template alloc_memory<DType>(send_count, sizeof(DType), ctx);
        auto recv_buf =
            devs[rank]->template alloc_memory<DType>(total_recv_count, sizeof(DType), ctx);
        send_buf.enqueue_write_sync(send_values.begin() + recv_offsets[rank],
                                    send_values.begin() + recv_offsets[rank] + send_count);
        recv_buf.enqueue_write_sync(recv_values.begin(), recv_values.begin() + total_recv_count);

        if (with_ipc)
            obj->template register_ipc_memories_data<DType>(ctx, rank, &recv_buf);

        obj->register_shared_memories_data(rank, std::move(send_buf), std::move(recv_buf));
    }
}

template <class DType, class Object>
void check_allgatherv_buffers(Object obj, size_t comm_size, std::vector<size_t> recv_counts) {
    std::stringstream ss;
    size_t total_recv_count = std::accumulate(recv_counts.begin(), recv_counts.end(), 0);
    std::vector<DType> expected_buf = get_initial_send_values<DType>(total_recv_count);

    for (size_t rank = 0; rank < comm_size; rank++) {
        ss << "\ncheck recv buffer for rank: " << rank;
        auto res = compare_buffers(expected_buf, obj->get_memory(rank, 1), ss);
        UT_ASSERT_OBJ(res, obj, ss.str());
    }
}
