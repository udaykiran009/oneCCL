#include <algorithm>
#include <sstream>

#include "native_device_api/l0/base_impl.hpp"

namespace native {

std::string CCL_API to_string(const ze_result_t result) {
    switch (result) {
        case ZE_RESULT_SUCCESS: return "ZE_RESULT_SUCCESS";
        case ZE_RESULT_NOT_READY: return "ZE_RESULT_NOT_READY";
        case ZE_RESULT_ERROR_UNINITIALIZED: return "ZE_RESULT_ERROR_UNINITIALIZED";
        case ZE_RESULT_ERROR_DEVICE_LOST: return "ZE_RESULT_ERROR_DEVICE_LOST";
        case ZE_RESULT_ERROR_UNSUPPORTED_FEATURE: return "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
        case ZE_RESULT_ERROR_INVALID_ARGUMENT: return "ZE_RESULT_ERROR_INVALID_ARGUMENT";
        case ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY: return "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY";
        case ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY: return "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY";
        case ZE_RESULT_ERROR_MODULE_BUILD_FAILURE:
            return "ZE_RESULT_ERROR_MODULE_BUILD_FAILURE";
            //        case  ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS :
            //            return "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS";
            //        case ZE_RESULT_ERROR_DEVICE_IS_IN_USE :
            //            return "ZE_RESULT_ERROR_DEVICE_IS_IN_USE ";
        case ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT: return "ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT ";
        case ZE_RESULT_ERROR_NOT_AVAILABLE: return "ZE_RESULT_ERROR_NOT_AVAILABLE ";
        case ZE_RESULT_ERROR_UNKNOWN: return "ZE_RESULT_ERROR_UNKNOWN ";
        default:
            throw std::runtime_error(std::string("Unknown ze_result_t value: ") +
                                     std::to_string(static_cast<int>(result)));
    }
    return "";
}

std::string CCL_API to_string(ze_device_type_t type) {
    switch (type) {
        case ZE_DEVICE_TYPE_GPU: return "ZE_DEVICE_TYPE_GPU";
        case ZE_DEVICE_TYPE_FPGA: return "ZE_DEVICE_TYPE_FPGA";
        default:
            assert(false && "incorrect ze_device_type_t type");
            throw std::runtime_error(std::string("Unknown ze_device_type_t value: ") +
                                     std::to_string(static_cast<int>(type)));
    }
    return "";
}

std::string to_string(ze_memory_access_capabilities_t cap) {
    std::string ret;
    if (cap & ZE_MEMORY_ACCESS_NONE) {
        ret += "ZE_MEMORY_ACCESS_NONE";
    }
    if (cap & ZE_MEMORY_ACCESS) {
        ret += ret.empty() ? "ZE_MEMORY_ACCESS" : "|ZE_MEMORY_ACCESS";
    }
    if (cap & ZE_MEMORY_ATOMIC_ACCESS) {
        ret += ret.empty() ? "ZE_MEMORY_ATOMIC_ACCESS" : "|ZE_MEMORY_ATOMIC_ACCESS";
    }
    if (cap & ZE_MEMORY_CONCURRENT_ACCESS) {
        ret += ret.empty() ? "ZE_MEMORY_CONCURRENT_ACCESS" : "|ZE_MEMORY_CONCURRENT_ACCESS";
    }
    if (cap & ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS) {
        ret += ret.empty() ? "ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS"
                           : "|ZE_MEMORY_CONCURRENT_ATOMIC_ACCESS";
    }
    return ret;
}

std::string CCL_API to_string(ze_memory_type_t type) {
    switch (type) {
        case ZE_MEMORY_TYPE_UNKNOWN: return "ZE_MEMORY_TYPE_UNKNOWN";
        case ZE_MEMORY_TYPE_HOST: return "ZE_MEMORY_TYPE_HOST";
        case ZE_MEMORY_TYPE_DEVICE: return "ZE_MEMORY_TYPE_DEVICE";
        case ZE_MEMORY_TYPE_SHARED: return "ZE_MEMORY_TYPE_SHARED";
        default:
            throw std::runtime_error(std::string("Unknown ze_memory_type_t value: ") +
                                     std::to_string(static_cast<int>(type)));
            break;
    }
    return "";
}

std::string CCL_API to_string(const ze_device_properties_t& device_properties,
                              const std::string& prefix) {
    std::stringstream ss;
    ss << "name: " << device_properties.name << prefix
       << "type: " << native::to_string(device_properties.type) << prefix
       << "vendor_id: " << device_properties.vendorId << prefix
       << "device_id: " << device_properties.deviceId << prefix
       << "uuid: " << device_properties.uuid.id << prefix
       << "isSubDevice: " << (bool)device_properties.isSubdevice;

    if (device_properties.isSubdevice) {
        ss << prefix << "subdevice_id: " << device_properties.subdeviceId;
    }

    ss << prefix << "unifiedMemorySupported:" << (bool)device_properties.unifiedMemorySupported
       << prefix << "onDemandPageFaults: " << (bool)device_properties.onDemandPageFaultsSupported
       << prefix << "coreClockRate: " << device_properties.coreClockRate << prefix
       << "maxCommandQueues: " << device_properties.maxCommandQueues << prefix
       << "numAsyncComputeEngines: " << device_properties.numAsyncComputeEngines << prefix
       << "numAsyncCopyEngines: " << device_properties.numAsyncCopyEngines << prefix
       << "maxCommandQueuePriority: " << device_properties.maxCommandQueuePriority << prefix
       << "numThreadsPerEU: " << device_properties.numThreadsPerEU << prefix
       << "physicalEUSimdWidth: " << device_properties.physicalEUSimdWidth << prefix
       << "numEUsPerSubslice: " << device_properties.numEUsPerSubslice << prefix
       << "numSubslicesPerSlice: " << device_properties.numSubslicesPerSlice << prefix
       << "umSlices: " << device_properties.numSlices;
    return ss.str();
}

std::string CCL_API to_string(const ze_device_memory_properties_t& device_mem_properties,
                              const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "maxClockRate: " << device_mem_properties.maxClockRate << prefix
       << "maxlBusWidth: " << device_mem_properties.maxBusWidth << prefix
       << "totalSize: " << device_mem_properties.totalSize;
    return ss.str();
}

std::string CCL_API to_string(const ze_device_memory_access_properties_t& mem_access_prop,
                              const std::string& prefix) {
    std::stringstream ss;
    ss << "hostAllocCapabilities: " << native::to_string(mem_access_prop.hostAllocCapabilities)
       << prefix
       << "deviceAllocCapabilities: " << native::to_string(mem_access_prop.deviceAllocCapabilities)
       << prefix << "sharedSingleDeviceAllocCapabilities: "
       << native::to_string(mem_access_prop.sharedSingleDeviceAllocCapabilities) << prefix
       << "sharedCrossDeviceAllocCapabilities: "
       << native::to_string(mem_access_prop.sharedCrossDeviceAllocCapabilities) << prefix
       << "sharedSystemAllocCapabilities: "
       << native::to_string(mem_access_prop.sharedSystemAllocCapabilities);
    return ss.str();
}

std::string CCL_API to_string(const ze_device_compute_properties_t& compute_properties,
                              const std::string& prefix) {
    std::stringstream ss;
    ss << prefix << "maxTotalGroupSize: " << compute_properties.maxTotalGroupSize << prefix
       << "maxGroupSizeX: " << compute_properties.maxGroupSizeX << prefix
       << "maxGroupSizeY: " << compute_properties.maxGroupSizeY << prefix
       << "maxGroupSizeZ: " << compute_properties.maxGroupSizeZ << prefix
       << "maxGroupCountX: " << compute_properties.maxGroupCountX << prefix
       << "maxGroupCountY: " << compute_properties.maxGroupCountY << prefix
       << "maxGroupCountZ: " << compute_properties.maxGroupCountZ << prefix
       << "maxSharedLocalMemory: " << compute_properties.maxSharedLocalMemory << prefix
       << "numSubGroupSizes: " << compute_properties.numSubGroupSizes << prefix
       << "subGroupSizes: ";
    std::copy(compute_properties.subGroupSizes,
              compute_properties.subGroupSizes + ZE_SUBGROUPSIZE_COUNT,
              std::ostream_iterator<uint32_t>(ss, ", "));
    return ss.str();
}

std::string CCL_API to_string(const ze_memory_allocation_properties_t& prop) {
    std::stringstream ss;
    ss << "version: " << prop.version << ", type: " << to_string(prop.type) <<
        //", device: " << prop.device <<
        ", id: " << prop.id;
    return ss.str();
}

std::string CCL_API to_string(const ze_device_p2p_properties_t& properties) {
    std::stringstream ss;
    ss << "version: " << properties.version
       << ", accessSupported: " << (bool)properties.accessSupported
       << ", atomicsSupported: " << (bool)properties.atomicsSupported;
    return ss.str();
}

std::string CCL_API to_string(const ze_ipc_mem_handle_t& handle) {
    std::stringstream ss;
    std::copy(
        handle.data, handle.data + ZE_MAX_IPC_HANDLE_SIZE, std::ostream_iterator<int>(ss, ", "));
    return ss.str();
}
} // namespace native
