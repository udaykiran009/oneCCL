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
bool bcast_checking_results(Object obj,
                            size_t num_thread,
                            int root,
                            size_t buffer_size,
                            std::stringstream& ss) {
    size_t corr_val = 0;
    try {
        for (size_t thread_idx = 0; thread_idx < num_thread; thread_idx++) {
            auto lambda =
                [&corr_val](
                    const int root, size_t thread_idx, size_t buffer_size, DType value) -> bool {
                corr_val++;
                if (corr_val > buffer_size)
                    corr_val = 1;
                if (value != static_cast<DType>(corr_val))
                    return false;
                return true;
            };

            obj->check_test_results(
                thread_idx, obj->output, 0 /*recv_mem*/, lambda, root, thread_idx, buffer_size);
        }
        return true;
    }
    catch (check_on_exception& ex) {
        obj->output << "Check results: \n";
        //printout
        obj->output << "Send memory:" << std::endl;
        obj->dump_memory_by_index(obj->output, 0 /*send_mem*/);
        obj->output << "\nRecv memory:" << std::endl;
        obj->dump_memory_by_index(obj->output, 1 /*recv_mem*/);

        ss << ex.what() << ", But expected: " << corr_val * num_thread << std::endl;
        return false;
    }
}
