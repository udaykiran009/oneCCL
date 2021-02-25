#pragma once

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

template <class DType>
class ring_bcast_single_process_fixture : public platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_bcast_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~ring_bcast_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_bcast.spv");
    }

    void TearDown() override {}
};

template <class DType>
class a2a_bcast_single_process_fixture : public platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    a2a_bcast_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~a2a_bcast_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/a2a_bcast.spv");
    }

    void TearDown() override {}
};

template <class DType>
class ring_bcast_multi_process_fixture : public multi_platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    ring_bcast_multi_process_fixture() : multi_platform_fixture(get_global_device_indices()) {}

    ~ring_bcast_multi_process_fixture() = default;

    void SetUp() override {
        multi_platform_fixture::SetUp();

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_bcast.spv");
    }

    void TearDown() override {}
};

template <class DType, class Object>
void alloc_and_fill_bcast_buffers(Object obj,
                                  size_t elem_count,
                                  size_t root,
                                  const std::vector<native::ccl_device_driver::device_ptr>& devs,
                                  std::shared_ptr<native::ccl_context> ctx,
                                  bool with_ipc = false) {
    std::vector<DType> send_values(elem_count);
    std::iota(send_values.begin(), send_values.end(), 1);
    std::vector<DType> recv_values(elem_count, 0);

    for (size_t rank = 0; rank < devs.size(); rank++) {
        auto buf = devs[rank]->template alloc_memory<DType>(elem_count, sizeof(DType), ctx);

        if (rank == root) {
            buf.enqueue_write_sync(send_values);
        }
        else {
            buf.enqueue_write_sync(recv_values);
        }

        if (with_ipc)
            obj->template register_ipc_memories_data<DType>(ctx, rank, &buf);

        obj->register_shared_memories_data(rank, std::move(buf));
    }
}

template <class DType, class Object>
void check_bcast_buffers(Object obj, size_t comm_size, size_t elem_count) {
    std::stringstream ss;
    bool res = true;

    std::vector<DType> expected_buf(elem_count);
    std::iota(expected_buf.begin(), expected_buf.end(), 1);

    for (size_t rank = 0; rank < comm_size; rank++) {
        auto recv_buf = obj->get_memory(rank, 0);

        if (recv_buf != expected_buf) {
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
