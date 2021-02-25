#pragma once

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

template <class DType>
class ring_alltoallv_single_process_fixture : public platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_alltoallv_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~ring_alltoallv_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_alltoallv.spv");
    }

    void TearDown() override {}
};

template <class DType>
class ring_alltoallv_multi_process_fixture : public multi_platform_fixture,
                                             public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_alltoallv_multi_process_fixture() : multi_platform_fixture(get_global_device_indices()) {}

    ~ring_alltoallv_multi_process_fixture() override {}

    void SetUp() override {
        multi_platform_fixture::SetUp();

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_alltoallv.spv");
    }

    void TearDown() override {}
};

template <class DType, class Object>
void alloc_and_fill_alltoallv_buffers(
    Object obj,
    std::vector<size_t> total_send_sizes,
    size_t total_recv_size,
    size_t tmp_buffer_size,
    std::vector<std::vector<DType>> send_values,
    std::vector<std::vector<DType>> recv_values,
    const std::vector<native::ccl_device_driver::device_ptr>& devs,
    std::shared_ptr<native::ccl_context> ctx,
    bool with_ipc = false) {
    //std::stringstream ss;

    for (size_t rank = 0; rank < devs.size(); rank++) {
        auto send_buf =
            devs[rank]->template alloc_memory<DType>(total_send_sizes[rank], sizeof(DType), ctx);
        auto recv_buf =
            devs[rank]->template alloc_memory<DType>(total_recv_size, sizeof(DType), ctx);
        auto tmp_buf =
            devs[rank]->template alloc_memory<DType>(tmp_buffer_size, sizeof(DType), ctx);

        send_buf.enqueue_write_sync(send_values[rank]);
        recv_buf.enqueue_write_sync(recv_values[rank]);

        if (with_ipc)
            obj->template register_ipc_memories_data<DType>(ctx, rank, &tmp_buf);

        obj->register_shared_memories_data(
            rank, std::move(send_buf), std::move(recv_buf), std::move(tmp_buf));

        // ss << "\nfill: tmp_buffer_size " << tmp_buffer_size << ", total_recv_size " << total_recv_size << ", total_size " << total_send_sizes[rank] << ", send_values[" << rank << "]: ";
        // std::copy(send_values[rank].begin(), send_values[rank].end(), std::ostream_iterator<DType>(ss, " "));
    }

    //std::cout << ss.str() << std::endl;
}

template <class DType, class Object>
void check_alltoallv_buffers(Object obj, size_t comm_size, size_t total_recv_count) {
    std::stringstream ss;
    bool res = true;

    std::vector<DType> expected_buf(total_recv_count);
    std::iota(expected_buf.begin(), expected_buf.end(), 0);

    for (size_t rank = 0; rank < comm_size; rank++) {
        auto recv_buf = obj->get_memory(rank, 1);
        if (recv_buf != expected_buf) {
            size_t idx;
            for (idx = 0; idx < expected_buf.size(); idx++) {
                if (expected_buf[idx] != recv_buf[idx])
                    break;
            }
            ss << "\nunexpected recv buffer for rank " << rank << ", index: " << idx
               << ", got: " << recv_buf[idx] << ", expected: " << expected_buf[idx];

            ss << "\nunexpected recv buffer for rank " << rank << ":\n";
            std::copy(recv_buf.begin(), recv_buf.end(), std::ostream_iterator<DType>(ss, " "));
            ss << "\nexpected:\n";
            std::copy(
                expected_buf.begin(), expected_buf.end(), std::ostream_iterator<DType>(ss, " "));
            res = false;
            break;
        }
    }

    UT_ASSERT_OBJ(res, obj, ss.str());
}
