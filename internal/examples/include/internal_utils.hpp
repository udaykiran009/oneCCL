#ifndef INTERNAL_UTILS_HPP
#define INTERNAL_UTILS_HPP

#include <algorithm>
#include <iterator>
#include <sstream>
#include <tuple>
#include <utility>

#include <mpi.h>
#include "base_utils.hpp"

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include <ze_api.h>
#include <CL/sycl/backend/level_zero.hpp>
#endif

namespace utils {

std::string to_string(const ccl::device_index_type& device_id) {
    std::stringstream ss;
    ss << "[" << std::get<ccl::device_index_enum::driver_index_id>(device_id) << ":"
       << std::get<ccl::device_index_enum::device_index_id>(device_id) << ":";

    auto subdevice_id = std::get<ccl::device_index_enum::subdevice_index_id>(device_id);
    if (subdevice_id == ccl::unused_index_value) {
        ss << "*";
    }
    else {
        ss << std::get<ccl::device_index_enum::subdevice_index_id>(device_id);
    }
    ss << "]";

    return ss.str();
}

ccl::device_index_type from_string(const std::string& device_id_str) {
    std::string::size_type from_pos = device_id_str.find('[');
    if (from_pos == std::string::npos) {
        throw std::invalid_argument(std::string("Cannot get ccl::device_index_type from input: ") +
                                    device_id_str + ". Index should be decorated by \"[...]\"");
    }

    if (device_id_str.size() == 1) {
        throw std::invalid_argument(
            std::string("Cannot get ccl::device_index_type from input, too less string: ") +
            device_id_str);
    }
    from_pos++;

    ccl::device_index_type path(
        ccl::unused_index_value, ccl::unused_index_value, ccl::unused_index_value);

    size_t cur_index = 0;
    do {
        std::string::size_type to_pos = device_id_str.find(':', from_pos);
        std::string::size_type count =
            (to_pos != std::string::npos ? to_pos - from_pos : std::string::npos);
        std::string index_string(device_id_str, from_pos, count);
        switch (cur_index) {
            case ccl::device_index_enum::driver_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "driver index invalid: ") +
                        device_id_str);
                }
                std::get<ccl::device_index_enum::driver_index_id>(path) = index;
                break;
            }
            case ccl::device_index_enum::device_index_id: {
                auto index = std::atoll(index_string.c_str());
                if (index < 0) {
                    throw std::invalid_argument(
                        std::string("Cannot get ccl::device_index_type from input, "
                                    "device index invalid: ") +
                        device_id_str);
                }
                std::get<ccl::device_index_enum::device_index_id>(path) = index;
                break;
            }
            case ccl::device_index_enum::subdevice_index_id: {
                auto index = std::atoll(index_string.c_str());
                std::get<ccl::device_index_enum::subdevice_index_id>(path) =
                    index < 0 ? ccl::unused_index_value : index;
                break;
            }
            default:
                throw std::invalid_argument(
                    std::string("Cannot get ccl::device_index_type from input, "
                                "unsupported format: ") +
                    device_id_str);
        }

        cur_index++;
        if (device_id_str.size() > to_pos) {
            to_pos++;
        }
        from_pos = to_pos;
    } while (from_pos < device_id_str.size());

    return path;
}

#ifdef CCL_ENABLE_ZE

template <>
void str_to_mset(const char* input, std::multiset<ccl::device_index_type>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.insert(utils::from_string(processes_input));
    }
}

std::shared_ptr<ccl::kvs> build_kvs(int mpi_rank) {
    std::shared_ptr<ccl::kvs> kvs_instance;
    ccl::kvs::address_type main_addr;
    if (mpi_rank == 0) {
        kvs_instance = ccl::create_main_kvs();
        main_addr = kvs_instance->get_address();
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
    }
    else {
        MPI_Bcast((void*)main_addr.data(), main_addr.size(), MPI_BYTE, 0, MPI_COMM_WORLD);
        kvs_instance = ccl::create_kvs(main_addr);
    }
    return kvs_instance;
}

void mpi_finalize() {
    int is_finalized = 0;
    MPI_Finalized(&is_finalized);

    if (!is_finalized)
        MPI_Finalize();
}

inline size_t take_mpi_rank_id_offest(const size_t mpi_rank_in_cluster,
                                      const int mpi_size,
                                      const size_t total_device_in_cluster) {
    if (mpi_size > 2) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - Only TWO processes support case !\n");
    }
    return total_device_in_cluster;
}

#ifdef CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_ZE
size_t get_sycl_device_id(const cl::sycl::device& device) {
    if (!device.is_gpu()) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - failed for sycl device: it is not gpu!");
    }
    size_t device_id = std::numeric_limits<size_t>::max();
    try {
        // extract native handle L0
        auto l0_handle = device.template get_native<cl::sycl::backend::ext_oneapi_level_zero>();

        ze_device_properties_t device_properties;
        ze_result_t ret = zeDeviceGetProperties(l0_handle, &device_properties);
        if (ret != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(std::string(__FUNCTION__) +
                                     " - zeDeviceGetProperties failed, error");
        }
        device_id = device_properties.deviceId;
    }
    catch (const cl::sycl::exception& e) {
        //TODO: errc::backend_mismatch
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 "- cannot retrieve l0 handle: " + e.what());
    }
    return device_id;
}

size_t get_sycl_subdevice_id(const cl::sycl::device& device) {
    if (!device.is_gpu()) {
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 " - failed for sycl device: it is not gpu!");
    }

    size_t subdevice_id = std::numeric_limits<size_t>::max();
    try {
        // extract native handle L0
        auto l0_handle = device.template get_native<cl::sycl::backend::ext_oneapi_level_zero>();

        ze_device_properties_t device_properties;
        ze_result_t ret = zeDeviceGetProperties(l0_handle, &device_properties);
        if (ret != ZE_RESULT_SUCCESS) {
            throw std::runtime_error(std::string(__FUNCTION__) +
                                     " - zeDeviceGetProperties failed, error");
        }

        if (!(device_properties.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)) {
            return ccl::unused_index_value;
        }

        subdevice_id = device_properties.subdeviceId;
    }
    catch (const cl::sycl::exception& e) {
        //TODO: errc::backend_mismatch
        throw std::runtime_error(std::string(__FUNCTION__) +
                                 "- cannot retrieve l0 handle: " + e.what());
    }
    return subdevice_id;
}
#endif // CCL_ENABLE_ZE

cl::sycl::device create_device_from_index(
    const ccl::device_index_type& device_vendor_id,
    const std::string& platform_name,
    cl::sycl::info::device_type type = info::device_type::gpu) {
    auto platforms = cl::sycl::platform::get_platforms();
    auto platform_it = std::find_if(
        platforms.begin(), platforms.end(), [platform_name](const cl::sycl::platform& pl) {
            return pl.get_info<cl::sycl::info::platform::name>().find(platform_name) !=
                   std::string::npos;
        });

    if (platform_it == platforms.end()) {
        std::stringstream ss;
        ss << "cannot find Level-Zero platform. Supported platforms are:\n";
        for (const auto& pl : platforms) {
            ss << "Platform:\nprofile: " << pl.get_info<cl::sycl::info::platform::profile>()
               << "\nversion: " << pl.get_info<cl::sycl::info::platform::version>()
               << "\nname: " << pl.get_info<cl::sycl::info::platform::name>()
               << "\nvendor: " << pl.get_info<cl::sycl::info::platform::vendor>();
        }

        throw std::runtime_error(std::string("Cannot find device by id: ") +
                                 utils::to_string(device_vendor_id) + ", reason:\n" + ss.str());
    }

    auto devices = platform_it->get_devices(type);
#ifdef CCL_ENABLE_ZE
    for (auto it_device = devices.begin(); it_device != devices.end(); ++it_device) {
        const cl::sycl::device& device = *it_device;
        if (!device.is_gpu()) {
            continue;
        }

        size_t device_id = get_sycl_device_id(device);
        ccl::device_index_type full_id(0, device_id, ccl::unused_index_value);

        if (full_id != device_vendor_id) {
            auto sub_devices =
                device.create_sub_devices<info::partition_property::partition_by_affinity_domain>(
                    info::partition_affinity_domain::next_partitionable);
            for (auto iter_subdevice = sub_devices.begin(); iter_subdevice != sub_devices.end();
                 iter_subdevice++) {
                size_t subdevice_id = get_sycl_subdevice_id(*iter_subdevice);
                std::get<ccl::device_index_enum::subdevice_index_id>(full_id) = subdevice_id;
                if (full_id == device_vendor_id) {
                    return *iter_subdevice;
                }
            }
        }
        else {
            return *it_device;
        }
    }
#endif
    std::stringstream ss;
    ss << "cannot find requested device. Supported devices are:\n";
    for (const auto& dev : devices) {
        ss << "Device:\nname: " << dev.get_info<cl::sycl::info::device::name>()
           << "\nvendor: " << dev.get_info<cl::sycl::info::device::vendor>()
           << "\nversion: " << dev.get_info<cl::sycl::info::device::version>()
           << "\nprofile: " << dev.get_info<cl::sycl::info::device::profile>();
    }

    throw std::runtime_error(std::string("Cannot find device by id: ") +
                             utils::to_string(device_vendor_id) + ", reason:\n" + ss.str());
}
#endif

ccl::process_device_indices_type extract_indices_for_threads(
    const size_t mpi_rank_in_cluster,
    const int current_mpi_rank,
    std::vector<std::string> thread_gpu_affinity,
    size_t& total_device_in_cluster,
    std::vector<size_t>& total_devices_in_process,
    std::map<size_t, std::vector<ccl::communicator::device_type>>& devices_for_current_mpi_rank) {
    ccl::process_device_indices_type thread_group_affinity;

    for (size_t thread_index = 0; thread_index < thread_gpu_affinity.size(); thread_index++) {
        ccl::device_indices_type device_group_affinity;
        str_to_mset<ccl::device_index_type>(
            thread_gpu_affinity[thread_index].c_str(), device_group_affinity, ',');

        std::cout << " Extracted GPU indices for thread by id: " << thread_index
                  << ", devices in threads count: " << device_group_affinity.size() << std::endl;
        total_device_in_cluster += device_group_affinity.size();
        total_devices_in_process[mpi_rank_in_cluster] += device_group_affinity.size();
        thread_group_affinity[thread_index] = device_group_affinity;

        if (mpi_rank_in_cluster == static_cast<size_t>(current_mpi_rank)) {
            for (auto device_vendor_id : device_group_affinity) {
                devices_for_current_mpi_rank[thread_index].push_back(
#ifdef CCL_ENABLE_SYCL
                    utils::create_device_from_index(device_vendor_id, "Level-Zero"));
#else
                    ::native::ccl_device_platform::get_platform().get_device(device_vendor_id));
#endif
            }
        }
    }
    return thread_group_affinity;
}

std::vector<ccl::communicator::device_type> set_union_devices_in_current_process(
    const std::map<size_t, std::vector<ccl::communicator::device_type>>& devices_for_mpi_rank) {
    std::vector<ccl::communicator::device_type> devices_in_process;
    for (auto& thread_devices : devices_for_mpi_rank) {
        devices_in_process.insert(
            devices_in_process.end(), thread_devices.second.begin(), thread_devices.second.end());
    }
    return devices_in_process;
}

#endif //CCL_ENABLE_ZE
} // namespace utils

#endif //INTERNAL_UTILS_HPP
