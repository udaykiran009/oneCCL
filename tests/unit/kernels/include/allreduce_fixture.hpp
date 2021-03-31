#pragma once

#include "../base_kernel_fixture.hpp"
#include "data_storage.hpp"

#include "kernels/shared.h"

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

    // Calculate size of segments used in allreduce kernel, tmp_buffer is allocated based on that
    const size_t tmp_buffer_size =
        ring_allreduce_get_tmp_buffer_size(elem_count, obj->get_comm_size());

    std::vector<DType> tmp_values(tmp_buffer_size, 0);

    // Fill recv_values and tmp_values with a predefined set of numbers, when the kernel is finished we check
    // whether some of these numbers have changed. This way we can identify out of bound writes performed by
    // GPU on recv and tmp buffers. This is really helpful to identify bug in our kernel since otherwise they
    // will be hidden as the drivers usually allocates more memory than requested, but this is not always the case
    for (size_t i = 0; i < obj->get_out_of_bound_payload_size(); ++i) {
        recv_values.push_back(i + 1);
        // Make values distinct from recv_values just in case
        tmp_values.push_back(2 * (i + 1));
    }

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
            tmp_buffer_size + obj->get_out_of_bound_payload_size(),
            sizeof(DType),
            ctx,
            mem_uncached_descr);

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

    // TODO: Make this code shared for all collectives
    /* Additional printout for debug purpose: unconditionally print the recv buffer we've got
       from the GPU */
    if (std::getenv("UT_DEBUG_OUTPUT") != nullptr) {
        std::stringstream out;

        for (size_t rank = 0; rank < obj->get_comm_size(); ++rank) {
            out << "Got from rank " << rank << " :";
            auto recv_mem = obj->get_memory(rank, 1);

            for (auto it = recv_mem.begin(); it != recv_mem.begin() + elem_count; ++it) {
                out << *it << " ";
            }

            out << "\n\n";
        }

        std::cout << out.str();
    }

    for (size_t rank = 0; rank < obj->get_comm_size(); rank++) {
        ss << "\nvalidating recv buffer for rank: " << rank;

        // TODO: make these checks shared for all collectives
        auto recv_mem = obj->get_memory(rank, 1);

        // Before comparing expected and received values make sure that
        // we didn't have any out of bound writes on recv memory(i.e. make sure that
        // our additional bytes at the end that we added hasn't been changed by the kernel)
        for (size_t i = 0; i < obj->get_out_of_bound_payload_size(); ++i) {
            if (recv_mem[elem_count + i] != static_cast<DType>(i + 1)) {
                ss << "\nvalidation check failed for recv for rank: " << rank;
                ss << "\ncheck value is changed at: " << (elem_count + i);
                UT_ASSERT_OBJ(false, obj, ss.str());
            }
        }

        // Remove our additional elements from the vector to do regular check in compare_buffers, we don't
        // need them anymore
        for (size_t i = 0; i < obj->get_out_of_bound_payload_size(); ++i)
            recv_mem.pop_back();

        const size_t tmp_buffer_size =
            ring_allreduce_get_tmp_buffer_size(elem_count, obj->get_comm_size());

        // Do a similar check for tmp_buffer
        auto tmp_mem = obj->get_memory(rank, 2);
        for (size_t i = 0; i < obj->get_out_of_bound_payload_size(); ++i) {
            if (tmp_mem[tmp_buffer_size + i] != static_cast<DType>(2 * (i + 1))) {
                ss << "\nvalidation check failed for tmp_buf for rank: " << rank;
                ss << "\ncheck value is changed at: " << (tmp_buffer_size + i);
                UT_ASSERT_OBJ(false, obj, ss.str());
            }
        }

        ss << "\ncheck recv buffer for rank: " << rank;
        auto res = compare_buffers(expected_buf, recv_mem, ss);
        UT_ASSERT_OBJ(res, obj, ss.str());
    }
}
