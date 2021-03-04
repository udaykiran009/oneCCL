#pragma once

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

template <class DType>
class ring_allreduce_single_process_fixture : public platform_fixture,
                                              public data_storage<typename DType::first_type> {
protected:
    using native_type = typename DType::first_type;
    using op_type = typename DType::second_type;
    using storage = data_storage<native_type>;

    ring_allreduce_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~ring_allreduce_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_allreduce.spv");
    }

    void TearDown() override {}
};

template <class DType>
class a2a_allreduce_single_process_fixture : public platform_fixture, public data_storage<DType> {
protected:
    using native_type = DType;
    using storage = data_storage<native_type>;

    a2a_allreduce_single_process_fixture() : platform_fixture(get_global_device_indices()) {}

    ~a2a_allreduce_single_process_fixture() = default;

    void SetUp() override {
        platform_fixture::SetUp();

        // get affinity for the first (only first process) in affinity mask
        const auto& local_affinity = cluster_device_indices[0];
        create_local_platform(local_affinity);

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        create_module_descr("kernels/a2a_allreduce.spv");
    }

    void TearDown() override {}
};

template <class DType>
class ring_allreduce_multi_process_fixture : public multi_platform_fixture,
                                             public data_storage<typename DType::first_type> {
protected:
    using native_type = typename DType::first_type;
    using op_type = typename DType::second_type;
    using storage = data_storage<native_type>;

    ring_allreduce_multi_process_fixture() : multi_platform_fixture(get_global_device_indices()) {}

    ~ring_allreduce_multi_process_fixture() override {}

    void SetUp() override {
        multi_platform_fixture::SetUp();

        // prepare preallocated data storage
        storage::initialize_data_storage(get_local_devices().size());

        // prepare binary kernel source
        create_module_descr("kernels/ring_allreduce.spv");
    }

    void TearDown() override {}
};

template <class DType, class Object>
void alloc_and_fill_allreduce_buffers(
    Object obj,
    size_t elem_count,
    const std::vector<native::ccl_device_driver::device_ptr>& devs,
    std::shared_ptr<native::ccl_context> ctx,
    bool with_ipc = false) {
    std::vector<DType> send_values = get_initial_send_values<DType>(elem_count);
    std::vector<DType> recv_values(elem_count, 0);

    size_t comm_size = devs.size();

    ze_device_mem_alloc_desc_t mem_uncached_descr{
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
        .ordinal = 0,
    };

    for (size_t rank = 0; rank < devs.size(); rank++) {
        auto send_buf = devs[rank]->template alloc_memory<DType>(elem_count, sizeof(DType), ctx);
        auto recv_buf = devs[rank]->template alloc_memory<DType>(elem_count, sizeof(DType), ctx);

        /* FIXME: use 2x size for tmp buffer for parallel recv and reduce+send */
        /* consider to remove tmp buffer further */
        auto temp_buf = devs[rank]->template alloc_memory<DType>(
            2 * elem_count / comm_size, sizeof(DType), ctx, mem_uncached_descr);

        send_buf.enqueue_write_sync(send_values);
        recv_buf.enqueue_write_sync(recv_values);

        if (with_ipc)
            obj->template register_ipc_memories_data<DType>(ctx, rank, &temp_buf);

        obj->register_shared_memories_data(
            rank, std::move(send_buf), std::move(recv_buf), std::move(temp_buf));
    }
}

template <class DType, class OpType, class Object>
void check_allreduce_buffers(Object obj, size_t comm_size, size_t elem_count) {
    std::stringstream ss;

    std::vector<DType> send_values = get_initial_send_values<DType>(elem_count);
    std::vector<DType> expected_buf(elem_count);

    for (size_t idx = 0; idx < elem_count; idx++) {
        constexpr auto op = OpType{};
        DType expected = op.init();
        for (size_t rank = 0; rank < comm_size; rank++) {
            expected = op(expected, static_cast<DType>(send_values[idx]));
        }
        expected_buf[idx] = expected;
    }

    for (size_t rank = 0; rank < comm_size; rank++) {
        ss << "\ncheck recv buffer for rank: " << rank;
        auto res = compare_buffers(expected_buf, obj->get_memory(rank, 1), ss);
        UT_ASSERT_OBJ(res, obj, ss.str());
    }
}
