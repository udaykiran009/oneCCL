#pragma once

#include "base_test.hpp"
#include "fixture_utils.hpp"

class platform_fixture : public testing::Test, public tracer {
public:
    template <class NativeType, class... Handles>
    void register_ipc_memories_data(std::shared_ptr<native::ccl_context> ctx,
                                    size_t thread_idx,
                                    Handles&&... h) {
        throw std::runtime_error("IPC mem is unsupported");
    }

    virtual size_t get_out_of_bound_payload_size() const {
        return 0;
    }

protected:
    using thread_device_indices_t = ccl::process_device_indices_type;

    platform_fixture(const std::string& cluster_affinity) : tracer() {
        // extract GPU affinities by processes using '#' separator from L0_CLUSTER_AFFINITY_MASK
        std::vector<std::string> process_group_gpu_affinity;
        utils::str_to_array<std::string>(cluster_affinity, process_group_gpu_affinity, '#');

        // extract  GPU affinities by thread inside process using '|' separator from L0_CLUSTER_AFFINITY_MASK
        for (size_t process_index = 0; process_index < process_group_gpu_affinity.size();
             process_index++) {
            std::vector<std::string> thread_group_gpu_affinity;
            utils::str_to_array<std::string>(
                process_group_gpu_affinity[process_index].c_str(), thread_group_gpu_affinity, '|');

            // extract  GPU affinities by single device inside thread using ',' separator from L0_CLUSTER_AFFINITY_MASK
            thread_device_indices_t single_thread_group_affinity;
            for (size_t thread_index = 0; thread_index < thread_group_gpu_affinity.size();
                 thread_index++) {
                ccl::device_indices_type device_group_affinity;
                utils::str_to_mset<ccl::device_index_type>(
                    thread_group_gpu_affinity[thread_index].c_str(), device_group_affinity, ',');

                single_thread_group_affinity[thread_index] = device_group_affinity;
            }

            cluster_device_indices[process_index] = single_thread_group_affinity;
        }
    }

    virtual ~platform_fixture() {}

    virtual void SetUp() override {
        if (cluster_device_indices.empty()) {
            std::cerr << "Set \"" << ut_device_affinity_mask_name << "\" to run tests. Exit"
                      << std::endl;
            exit(-1);
        }

        create_global_platform();
    }

    // Creators
    void create_global_platform() {
        global_platform = native::ccl_device_platform::create();

        auto driver = global_platform->get_driver(0);
        const auto& dev_map = driver->get_devices();
        for (const auto& dev_pair : dev_map) {
            global_devices.push_back(dev_pair.second);
            const auto subdev_map = dev_pair.second->get_subdevices();
            for (const auto& subdev_pair : subdev_map) {
                global_devices.push_back(subdev_pair.second);
            }
        }
    }

    void create_local_platform(const ccl::process_device_indices_type& local_process_affinity =
                                   ccl::process_device_indices_type()) {
        local_platform_device_indices.clear();
        try {
            // make flat device indices list
            for (const auto& thread_set : local_process_affinity) {
                for (const auto& devices_set : thread_set.second) {
                    local_platform_device_indices.insert(devices_set);
                }
            }

            local_platform = native::ccl_device_platform::create(local_platform_device_indices);

            // fill devices array
            devices.reserve(local_platform_device_indices.size());
            for (const auto& index : local_platform_device_indices) {
                devices.push_back(local_platform->get_device(index));
                set_of_devices.insert(local_platform->get_device(index));
            }
        }
        catch (...) {
            local_platform_device_indices.clear();
            throw;
        }
    }

    void create_kernel(size_t device_index, const std::string& kernel_name) {
        if (device_index >= devices.size()) {
            throw std::runtime_error(
                std::string(__PRETTY_FUNCTION__) +
                " - invalid device index requested: " + std::to_string(device_index) +
                ", total devices count: " + std::to_string(devices.size()));
        }

        //prepare kernels
        ze_kernel_desc_t desc = {
            .stype = ZE_STRUCTURE_TYPE_KERNEL_DESC,
            .pNext = nullptr,
            .flags = 0,
        };
        desc.pKernelName = kernel_name.c_str();

        this->output << "KERNEL_NAME: " << desc.pKernelName << std::endl;

        native::ccl_device& device = *(devices[device_index]);
        native::ccl_device::device_module& module = *(this->device_modules.find(&device)->second);

        this->output << "Preparing kernels params: name of kernel: " << desc.pKernelName << "\n"
                     << "  device id: " << ccl::to_string(device.get_device_path()) << "\n"
                     << "  rank: " << device_index << std::endl;

        ze_kernel_handle_t handle = nullptr;
        try {
            ze_result_t result = zeKernelCreate(module.handle, &desc, &handle);
            if (result != ZE_RESULT_SUCCESS) {
                throw std::runtime_error(std::string("Cannot create kernel: ") + desc.pKernelName +
                                         ", error: " + native::to_string(result));
            }

            kernels.emplace(device_index, std::move(handle));
        }
        catch (const std::exception& ex) {
            throw std::runtime_error(std::string("Error: ") + ex.what());
        }
    }

    // Getters
    const std::map<size_t, thread_device_indices_t>& get_cluster_platform_device_indices() const {
        return cluster_device_indices;
    }

    const ccl::device_indices_type& get_local_platform_device_indices() const {
        return local_platform_device_indices;
    }

    const std::vector<native::ccl_device_driver::device_ptr>& get_local_devices() const {
        return devices;
    }

    const std::set<native::ccl_device_driver::device_ptr>& get_unique_local_devices() const {
        return set_of_devices;
    }

    const std::vector<native::ccl_device_driver::device_ptr>& get_global_devices() const {
        return global_devices;
    }

    ze_kernel_handle_t get_kernel(size_t device_index) const {
        auto it = kernels.find(device_index);
        if (it == kernels.end()) {
            throw std::runtime_error(
                std::string(__PRETTY_FUNCTION__) +
                " - invalid device index requested: " + std::to_string(device_index) +
                ", total kernels count: " + std::to_string(kernels.size()));
        }
        return it->second;
    }

    // Utils
    void create_module_descr(const std::string& module_path, bool replace = false) {
        ze_module_desc_t module_description;
        std::vector<uint8_t> binary_module_file;
        try {
            binary_module_file = utils::load_binary_file(module_path);
            module_description.stype = ZE_STRUCTURE_TYPE_MODULE_DESC;
            module_description.pNext = nullptr;
            module_description.format = ZE_MODULE_FORMAT_IL_SPIRV;
            module_description.inputSize = static_cast<uint32_t>(binary_module_file.size());
            module_description.pInputModule = binary_module_file.data();
            module_description.pBuildFlags = nullptr;
            module_description.pConstants = nullptr;
        }
        catch (const std::exception& ex) {
            UT_ASSERT(false, "Cannot load kernel: " << module_path << "\nError: " << ex.what());
            throw;
        }

        // compile test module per device
        std::size_t hash = std::hash<std::string>{}(module_path);
        auto it = local_platform->drivers.find(0);
        UT_ASSERT(it != local_platform->drivers.end(), "Driver idx 0 must exist");
        for (auto& dev_it : it->second->devices) {
            auto dev = dev_it.second;
            try {
                if (!replace) {
                    device_modules.emplace(
                        dev.get(),
                        dev->create_module(module_description, hash, dev->get_default_context()));
                }
                else {
                    device_modules.erase(dev.get());
                    device_modules.emplace(
                        dev.get(),
                        dev->create_module(module_description, hash, dev->get_default_context()));
                }
            }
            catch (const std::exception& ex) {
                UT_ASSERT(false,
                          "Cannot load module on device: " << dev->to_string()
                                                           << "\nError: " << ex.what());
            }

            //create for subdevices
            auto& subdevices = dev->get_subdevices();
            for (auto& subdev : subdevices) {
                try {
                    if (!replace) {
                        device_modules.emplace(
                            subdev.second.get(),
                            subdev.second->create_module(
                                module_description, hash, subdev.second->get_default_context()));
                    }
                    else {
                        device_modules.erase(subdev.second.get());
                        device_modules.emplace(
                            subdev.second.get(),
                            subdev.second->create_module(
                                module_description, hash, subdev.second->get_default_context()));
                    }
                }
                catch (const std::exception& ex) {
                    UT_ASSERT(false,
                              "Cannot load module on SUB device: " << subdev.second->to_string()
                                                                   << "\nError: " << ex.what());
                }
            }
        }
    }

    std::map<size_t, thread_device_indices_t> cluster_device_indices;

    std::shared_ptr<native::ccl_device_platform> global_platform;
    std::shared_ptr<native::ccl_device_platform> local_platform;

    std::map<native::ccl_device*, native::ccl_device::device_module_ptr> device_modules;

private:
    ccl::device_indices_type local_platform_device_indices;
    std::vector<native::ccl_device_driver::device_ptr> global_devices;
    std::vector<native::ccl_device_driver::device_ptr> devices;
    std::set<native::ccl_device_driver::device_ptr> set_of_devices;
    std::map<size_t /*devices element index*/, ze_kernel_handle_t> kernels;
};
