#pragma once

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

template <class DType>
class ring_allreduce_single_process_fixture : public platform_fixture,
                                              public data_storage<typename DType::first_type> {
public:
    // Specify the number of values we append to recv and tmp buffers in order to check for out of bound
    // writes in GPU kernel
    size_t get_out_of_bound_payload_size() const override {
        return 128;
    }

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
    obj->output << "Number of elements to test: " << elem_count << std::endl;

    std::vector<DType> send_values = get_initial_send_values<DType>(elem_count);
    std::vector<DType> recv_values(elem_count, 0);
    auto tmp_values =
        get_temp_buffer_vec<DType>(test_coll_type::allreduce, elem_count, obj->get_comm_size());

    append_out_of_bound_check_data(recv_values, tmp_values);

    ze_device_mem_alloc_desc_t mem_uncached_descr{
        .stype = ZE_STRUCTURE_TYPE_DEVICE_MEM_ALLOC_DESC,
        .pNext = NULL,
        .flags = ZE_DEVICE_MEM_ALLOC_FLAG_BIAS_UNCACHED,
        .ordinal = 0,
    };

    for (size_t rank = 0; rank < devs.size(); rank++) {
        auto send_buf = devs[rank]->template alloc_memory<DType>(elem_count, sizeof(DType), ctx);
        auto recv_buf = devs[rank]->template alloc_memory<DType>(
            elem_count + obj->get_out_of_bound_payload_size(), sizeof(DType), ctx);
        auto temp_buf = devs[rank]->template alloc_memory<DType>(
            tmp_values.size(), sizeof(DType), ctx, mem_uncached_descr);

        send_buf.enqueue_write_sync(send_values);
        recv_buf.enqueue_write_sync(recv_values);
        temp_buf.enqueue_write_sync(tmp_values);

        if (with_ipc)
            obj->template register_ipc_memories_data<DType>(ctx, rank, &temp_buf);

        obj->register_shared_memories_data(
            rank, std::move(send_buf), std::move(recv_buf), std::move(temp_buf));
    }
}

template <class DType, class OpType, class Object>
void check_allreduce_buffers(Object obj, size_t elem_count) {
    std::stringstream ss;

    std::vector<DType> send_values = get_initial_send_values<DType>(elem_count);
    std::vector<DType> expected_buf(elem_count);

    for (size_t idx = 0; idx < elem_count; idx++) {
        constexpr auto op = OpType{};
        DType expected = op.init();
        for (size_t rank = 0; rank < obj->get_comm_size(); rank++) {
            expected = op(expected, static_cast<DType>(send_values[idx]));
        }
        expected_buf[idx] = expected;
    }

    print_recv_data(obj, elem_count, /* recv_mem idx */ 1);

    for (size_t rank = 0; rank < obj->get_comm_size(); rank++) {
        ss << "\nvalidating recv and tmp buffers for rank: " << rank;

        auto recv_mem = obj->get_memory(rank, 1);
        auto tmp_mem = obj->get_memory(rank, 2);

        auto p = check_out_of_bound_data(recv_mem, elem_count, rank, tmp_mem);
        UT_ASSERT_OBJ(p.first, obj, p.second);

        ss << "\ncheck recv buffer for rank: " << rank;
        auto res = compare_buffers(expected_buf, recv_mem, ss);
        UT_ASSERT_OBJ(res, obj, ss.str());
    }
}
