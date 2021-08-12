#include <array>
#include <iomanip>
#include <iostream>
#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>
#include <stdlib.h>
#include <string>
#include <sstream>
#include <vector>

#include "options.hpp"

#define zeCall(myZeCall) \
    do { \
        if (myZeCall != ZE_RESULT_SUCCESS) { \
            std::cout << "Error at " << #myZeCall << ": " << __FUNCTION__ << ": " << std::dec \
                      << __LINE__ << "\n"; \
            std::terminate(); \
        } \
    } while (0);

std::vector<std::string> device_ids;
std::vector<std::string> device_names;
std::vector<uint32_t> device_slices;
std::vector<uint32_t> device_subslices;
std::vector<uint32_t> device_eu_per_subslice;
std::vector<uint32_t> device_max_compute_units;
std::vector<uint32_t> device_threads_per_eu;
std::vector<uint32_t> device_physical_eu_simd_widths;
std::vector<uint32_t> device_max_total_group_sizes;
std::vector<uint32_t> device_max_group_x_sizes;
std::vector<uint32_t> device_max_group_y_sizes;
std::vector<uint32_t> device_max_group_z_sizes;
std::vector<uint32_t> device_max_group_x_counts;
std::vector<uint32_t> device_max_group_y_counts;
std::vector<uint32_t> device_max_group_z_counts;
std::vector<uint32_t> device_max_hw_contexts;
std::vector<uint32_t> device_core_clock_rates;
std::vector<std::string> device_max_cmd_queue_priority;
std::vector<std::string> device_max_mem_alloc_sizes;
std::vector<std::string> device_max_shared_local_memory;
std::vector<std::vector<std::string>> device_mem_names;
std::vector<std::vector<std::string>> device_mem_sizes;
std::vector<std::vector<uint32_t>> device_mem_bus_widths;
std::vector<std::vector<uint32_t>> device_mem_clock_rates;
std::vector<std::vector<std::string>> device_cache_sizes;
std::vector<std::vector<std::string>> device_cache_ucc;
std::vector<std::vector<uint32_t>> device_queue_group_nums;
std::vector<std::vector<std::string>> device_queue_group_types;
std::vector<std::string> device_alloc_capabilities_rw;
std::vector<std::string> device_alloc_capabilities_atomic;
std::vector<std::string> device_alloc_capabilities_concurrent;
std::vector<std::string> device_alloc_capabilities_concurrent_atomic;

std::string human_bytes(size_t bytes) {
    constexpr std::array<const char*, 5> suffix{ "B", "KB", "MB", "GB", "TB" };
    double new_size = bytes;
    size_t id = 0;
    std::stringstream ss;

    if (bytes > 1024) {
        for (; (bytes / 1024) > 0 && id < suffix.size() - 1; id++, bytes /= 1024)
            new_size = bytes / 1024.0;
    }

    ss << std::fixed << std::setprecision(2) << new_size << " " << suffix[id];
    return ss.str();
}

void get_device_queue_group_info(const ze_device_handle_t& device) {
    uint32_t queue_group_count = 0;
    zeCall(zeDeviceGetCommandQueueGroupProperties(device, &queue_group_count, nullptr));
    std::vector<ze_command_queue_group_properties_t> queue_group_props(queue_group_count);
    zeCall(zeDeviceGetCommandQueueGroupProperties(
        device, &queue_group_count, queue_group_props.data()));

    if (device_queue_group_nums.size() < queue_group_count) {
        device_queue_group_nums.resize(queue_group_count);
        device_queue_group_types.resize(queue_group_count);
    }

    for (uint32_t queue_group_idx = 0; queue_group_idx < queue_group_count; ++queue_group_idx) {
        device_queue_group_nums[queue_group_idx].push_back(
            queue_group_props[queue_group_idx].numQueues);
        std::string queue_type = "other";
        if (queue_group_props[queue_group_idx].flags &
            ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            queue_type = "compute";
        }
        else if (queue_group_props[queue_group_idx].flags &
                 ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) {
            queue_type = "copy";
        }
        device_queue_group_types[queue_group_idx].push_back(queue_type);
    }
}

void get_device_cache_info(const ze_device_handle_t& device) {
    uint32_t cache_count = 0;
    zeCall(zeDeviceGetCacheProperties(device, &cache_count, nullptr));
    std::vector<ze_device_cache_properties_t> cache_props(cache_count);
    zeCall(zeDeviceGetCacheProperties(device, &cache_count, cache_props.data()));

    if (device_cache_sizes.size() < cache_count) {
        device_cache_sizes.resize(cache_count);
        device_cache_ucc.resize(cache_count);
    }

    for (uint32_t cache_idx = 0; cache_idx < cache_count; ++cache_idx) {
        device_cache_sizes[cache_idx].push_back(human_bytes(cache_props[cache_idx].cacheSize));
        std::string user_cache_control_is_supported = "no";
        if (cache_props[cache_idx].flags & ZE_DEVICE_CACHE_PROPERTY_FLAG_USER_CONTROL) {
            user_cache_control_is_supported = "yes";
        }
        device_cache_ucc[cache_idx].push_back(user_cache_control_is_supported);
    }
}

void get_device_memory_access_info(const ze_device_handle_t& device) {
    ze_device_memory_access_properties_t mem_access_prop;
    zeCall(zeDeviceGetMemoryAccessProperties(device, &mem_access_prop));

    auto device_alloc_flags = mem_access_prop.deviceAllocCapabilities;
    if (device_alloc_flags) {
        std::string rw_access_is_supported =
            (device_alloc_flags & ZE_MEMORY_ACCESS_CAP_FLAG_RW) ? "yes" : "no";
        device_alloc_capabilities_rw.push_back(rw_access_is_supported);
        std::string atomic_access_is_supported =
            (device_alloc_flags & ZE_MEMORY_ACCESS_CAP_FLAG_ATOMIC) ? "yes" : "no";
        device_alloc_capabilities_atomic.push_back(atomic_access_is_supported);
        std::string concurrent_access_is_supported =
            (device_alloc_flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT) ? "yes" : "no";
        device_alloc_capabilities_concurrent.push_back(concurrent_access_is_supported);
        std::string concurrent_atomic_access_is_supported =
            (device_alloc_flags & ZE_MEMORY_ACCESS_CAP_FLAG_CONCURRENT_ATOMIC) ? "yes" : "no";
        device_alloc_capabilities_concurrent_atomic.push_back(
            concurrent_atomic_access_is_supported);
    }
}

void get_device_memory_info(const ze_device_handle_t& device) {
    uint32_t mem_count = 0;
    zeCall(zeDeviceGetMemoryProperties(device, &mem_count, nullptr));
    std::vector<ze_device_memory_properties_t> mem_props(mem_count);
    zeCall(zeDeviceGetMemoryProperties(device, &mem_count, mem_props.data()));

    if (device_mem_names.size() < mem_count) {
        device_mem_names.resize(mem_count);
        device_mem_sizes.resize(mem_count);
        device_mem_bus_widths.resize(mem_count);
        device_mem_clock_rates.resize(mem_count);
    }

    for (uint32_t mem_idx = 0; mem_idx < mem_count; ++mem_idx) {
        device_mem_names[mem_idx].push_back(mem_props[mem_idx].name);
        device_mem_sizes[mem_idx].push_back(human_bytes(mem_props[mem_idx].totalSize));
        device_mem_bus_widths[mem_idx].push_back(mem_props[mem_idx].maxBusWidth);
        device_mem_clock_rates[mem_idx].push_back(mem_props[mem_idx].maxClockRate);
    }
}

void get_device_info(const ze_device_handle_t& device) {
    ze_device_properties_t device_prop;
    zeCall(zeDeviceGetProperties(device, &device_prop));
    device_names.push_back(device_prop.name);
    device_slices.push_back(device_prop.numSlices);
    device_subslices.push_back(device_prop.numSubslicesPerSlice);
    device_eu_per_subslice.push_back(device_prop.numEUsPerSubslice);
    device_max_compute_units.push_back(device_prop.numSlices * device_prop.numSubslicesPerSlice *
                                       device_prop.numEUsPerSubslice);
    device_threads_per_eu.push_back(device_prop.numThreadsPerEU);
    device_physical_eu_simd_widths.push_back(device_prop.physicalEUSimdWidth);
    device_max_hw_contexts.push_back(device_prop.maxHardwareContexts);
    device_core_clock_rates.push_back(device_prop.coreClockRate);
    std::string queue_priority = "unknown";
    if (device_prop.maxCommandQueuePriority == ZE_COMMAND_QUEUE_PRIORITY_NORMAL)
        queue_priority = "normal";
    else if (device_prop.maxCommandQueuePriority == ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_LOW)
        queue_priority = "low";
    else if (device_prop.maxCommandQueuePriority == ZE_COMMAND_QUEUE_PRIORITY_PRIORITY_HIGH)
        queue_priority = "high";
    device_max_cmd_queue_priority.push_back(queue_priority);
    device_max_mem_alloc_sizes.push_back(human_bytes(device_prop.maxMemAllocSize));

    ze_device_compute_properties_t device_compute_prop;
    zeCall(zeDeviceGetComputeProperties(device, &device_compute_prop));
    device_max_total_group_sizes.push_back(device_compute_prop.maxTotalGroupSize);
    device_max_group_x_sizes.push_back(device_compute_prop.maxGroupSizeX);
    device_max_group_y_sizes.push_back(device_compute_prop.maxGroupSizeY);
    device_max_group_z_sizes.push_back(device_compute_prop.maxGroupSizeZ);
    device_max_group_x_counts.push_back(device_compute_prop.maxGroupCountX);
    device_max_group_y_counts.push_back(device_compute_prop.maxGroupCountY);
    device_max_group_z_counts.push_back(device_compute_prop.maxGroupCountZ);
    device_max_shared_local_memory.push_back(human_bytes(device_compute_prop.maxSharedLocalMemory));

    get_device_memory_info(device);
    get_device_cache_info(device);
    get_device_queue_group_info(device);
    get_device_memory_access_info(device);
}

template <class vector_t>
void print_line(const vector_t& props, const std::string& title) {
    constexpr int title_col_width = 31;
    constexpr int col_width = 11;

    if (!title.empty()) {
        std::cout << std::left << std::setw(title_col_width) << std::string(title + ":");
        for (const auto& prop : props) {
            std::cout << std::left << std::setw(col_width) << prop;
        }
    }
    else {
        std::string sep(title_col_width + col_width * props.size(), '-');
        std::cout << std::left << sep;
    }
    std::cout << std::endl;
}

void print() {
    print_line(device_ids, "device index");
    std::cout << std::endl;

    std::cout << "Compute properties:" << std::endl;
    print_line(device_ids, "");
    print_line(device_slices, "number of slices");
    print_line(device_subslices, "number of subslices per slice");
    print_line(device_eu_per_subslice, "number of EUs per subslice");
    print_line(device_max_compute_units, "max compute units");
    print_line(device_threads_per_eu, "number of threads per EU");
    print_line(device_physical_eu_simd_widths, "physical EU simd width");
    print_line(device_max_total_group_sizes, "max total group size (x+y+z)");
    print_line(device_max_group_x_sizes, "max group size x");
    print_line(device_max_group_y_sizes, "max group size y");
    print_line(device_max_group_z_sizes, "max group size z");
    print_line(device_max_group_x_counts, "max group count x");
    print_line(device_max_group_y_counts, "max group count y");
    print_line(device_max_group_z_counts, "max group count z");
    print_line(device_max_hw_contexts, "max logical hw contexts");
    print_line(device_core_clock_rates, "core clock rate");
    print_line(device_max_cmd_queue_priority, "max cmd queues priority");
    std::cout << std::endl;

    if (device_mem_names.size() > 0) {
        std::cout << "Memory properties:" << std::endl;
        print_line(device_ids, "");
        print_line(device_max_mem_alloc_sizes, "max memory allocation size");
        print_line(device_max_shared_local_memory, "max shared local memory");
        for (size_t vec_idx = 0; vec_idx < device_mem_names.size(); ++vec_idx) {
            print_line(device_mem_names[vec_idx], "memory #" + std::to_string(vec_idx) + " name");
            print_line(device_mem_sizes[vec_idx], "memory #" + std::to_string(vec_idx) + " size");
            print_line(device_mem_bus_widths[vec_idx],
                       "memory #" + std::to_string(vec_idx) + " bus width");
            print_line(device_mem_clock_rates[vec_idx],
                       "memory #" + std::to_string(vec_idx) + " clock rate");
        }
        std::cout << std::endl;
    }

    if (device_alloc_capabilities_rw.size() > 0) {
        std::cout << "Memory access properties (for device allocation):" << std::endl;
        print_line(device_ids, "");
        print_line(device_alloc_capabilities_rw, "load/store support");
        print_line(device_alloc_capabilities_atomic, "atomic support");
        print_line(device_alloc_capabilities_concurrent, "concurrent support");
        print_line(device_alloc_capabilities_concurrent_atomic, "concurrent atomic support");
        std::cout << std::endl;
    }

    if (device_cache_sizes.size() > 0) {
        std::cout << "Cache properties:" << std::endl;
        print_line(device_ids, "");
        for (size_t vec_idx = 0; vec_idx < device_cache_sizes.size(); ++vec_idx) {
            print_line(device_cache_sizes[vec_idx], "cache #" + std::to_string(vec_idx) + " size");
            print_line(device_cache_ucc[vec_idx],
                       "cache #" + std::to_string(vec_idx) + " UCC support");
        }
        std::cout << std::endl;
    }

    if (device_queue_group_nums.size() > 0) {
        std::cout << "Queue group properties:" << std::endl;
        print_line(device_ids, "");
        for (size_t vec_idx = 0; vec_idx < device_queue_group_nums.size(); ++vec_idx) {
            print_line(device_queue_group_nums[vec_idx],
                       "queue #" + std::to_string(vec_idx) + " group size");
            print_line(device_queue_group_types[vec_idx],
                       "queue #" + std::to_string(vec_idx) + " group type");
        }
    }
}

int main(int argc, char* argv[]) {
    parse_options(argc, argv);

    zeCall(zeInit(0));

    uint32_t drivers_count = 0;
    zeCall(zeDriverGet(&drivers_count, nullptr));
    std::vector<ze_driver_handle_t> drivers(drivers_count);
    zeCall(zeDriverGet(&drivers_count, drivers.data()));

    for (uint32_t driver_idx = 0; driver_idx < drivers_count; ++driver_idx) {
        uint32_t devices_count = 0;
        zeCall(zeDeviceGet(drivers[driver_idx], &devices_count, nullptr));
        std::vector<ze_device_handle_t> devices(devices_count);
        zeCall(zeDeviceGet(drivers[driver_idx], &devices_count, devices.data()));

        for (uint32_t device_idx = 0, total_device_idx = 0; device_idx < devices_count;
             ++device_idx, ++total_device_idx) {
            if (options.show_device_with_idx >= 0 && options.show_device_with_idx != device_idx) {
                continue;
            }
            device_ids.push_back(std::to_string(device_idx));
            get_device_info(devices[device_idx]);

            uint32_t subdevices_count = 0;
            zeCall(zeDeviceGetSubDevices(devices[device_idx], &subdevices_count, nullptr));
            if (subdevices_count > 0 && !options.show_root_device_only) {
                std::vector<ze_device_handle_t> subdevices(subdevices_count);
                zeCall(zeDeviceGetSubDevices(
                    devices[device_idx], &subdevices_count, subdevices.data()));
                for (uint32_t subdevice_idx = 0; subdevice_idx < subdevices_count;
                     ++subdevice_idx) {
                    device_ids.push_back(std::to_string(device_idx) + ":" +
                                         std::to_string(subdevice_idx));
                    get_device_info(subdevices[subdevice_idx]);
                }
            }
            std::cout << "device #" << device_idx << ": " << device_names[total_device_idx]
                      << std::endl;
        }
    }

    std::cout << std::endl;
    print();
}
