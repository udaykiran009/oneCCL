#include <algorithm>
#include <fstream>

#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/entry/gpu/ze_primitives.hpp"

namespace ccl {

namespace ze {

void load_module(std::string dir,
                 std::string file_name,
                 ze_device_handle_t device,
                 ze_context_handle_t context,
                 ze_module_handle_t* module) {
    LOG_DEBUG("module loading started: directory: ", dir, ", file: ", file_name);

    if (!dir.empty()) {
        if (*dir.rbegin() != '/') {
            dir += '/';
        }
    }

    std::string file_path = dir + file_name;
    std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary);
    if (!file.good() || dir.empty() || file_name.empty()) {
        CCL_THROW("failed to load module: file: ", file_path);
    }

    file.seekg(0, file.end);
    size_t filesize = file.tellg();
    file.seekg(0, file.beg);

    std::vector<uint8_t> module_data(filesize);
    file.read(reinterpret_cast<char*>(module_data.data()), filesize);
    file.close();

    ze_module_desc_t desc = {};
    ze_module_format_t format = ZE_MODULE_FORMAT_IL_SPIRV;
    desc.format = format;
    desc.pInputModule = reinterpret_cast<const uint8_t*>(module_data.data());
    desc.inputSize = module_data.size();
    ZE_CALL(zeModuleCreate, (context, device, &desc, module, nullptr));
    LOG_DEBUG("module loading completed: directory: ", dir, ", file: ", file_name);
}

void create_kernel(ze_module_handle_t module, std::string kernel_name, ze_kernel_handle_t* kernel) {
    ze_kernel_desc_t desc = default_kernel_desc;
    // convert to lowercase
    std::transform(kernel_name.begin(), kernel_name.end(), kernel_name.begin(), ::tolower);
    desc.pKernelName = kernel_name.c_str();
    ze_result_t res = zeKernelCreate(module, &desc, kernel);
    if (res != ZE_RESULT_SUCCESS) {
        CCL_THROW("error at zeKernelCreate: kernel name: ", kernel_name, " ret: ", to_string(res));
    }
}

void get_suggested_group_size(ze_kernel_handle_t kernel,
                              size_t count,
                              ze_group_size_t* group_size) {
    CCL_ASSERT(count > 0, "count == 0");
    ZE_CALL(zeKernelSuggestGroupSize,
            (kernel,
             count,
             1,
             1,
             &group_size->groupSizeX,
             &group_size->groupSizeY,
             &group_size->groupSizeZ));
    CCL_THROW_IF_NOT(group_size->groupSizeX >= 1,
                     "wrong group size calculation: group size: ",
                     to_string(*group_size),
                     ", count: ",
                     count);
}

void get_suggested_group_count(const ze_group_size_t& group_size,
                               size_t count,
                               ze_group_count_t* group_count) {
    group_count->groupCountX = count / group_size.groupSizeX;
    group_count->groupCountY = 1;
    group_count->groupCountZ = 1;

    auto rem = count % group_size.groupSizeX;
    CCL_THROW_IF_NOT(group_count->groupCountX >= 1 && rem == 0,
                     "wrong group count calculation: group size: ",
                     to_string(group_size),
                     ", group count: ",
                     to_string(*group_count),
                     ", count: ",
                     std::to_string(count));
}

void set_kernel_args(ze_kernel_handle_t kernel, const ze_kernel_args_t& kernel_args) {
    uint32_t idx = 0;
    for (const auto& arg : kernel_args) {
        auto res = zeKernelSetArgumentValue(kernel, idx, arg.first, arg.second);
        if (res != ZE_RESULT_SUCCESS) {
            CCL_THROW("zeKernelSetArgumentValue failed with error ",
                      to_string(res),
                      " on idx ",
                      idx,
                      " with value ",
                      *((void**)arg.second));
        }
        ++idx;
    }
}

void get_num_queue_groups(ze_device_handle_t device, uint32_t* num) {
    *num = 0;
    ZE_CALL(zeDeviceGetCommandQueueGroupProperties, (device, num, nullptr));
    CCL_THROW_IF_NOT(*num != 0, "no queue groups found");
}

void get_queues_properties(ze_device_handle_t device,
                           uint32_t num_queue_groups,
                           ze_queue_properties_t* props) {
    props->resize(num_queue_groups);
    ZE_CALL(zeDeviceGetCommandQueueGroupProperties, (device, &num_queue_groups, props->data()));
}

void get_comp_queue_ordinal(ze_device_handle_t device,
                            const ze_queue_properties_t& props,
                            uint32_t* ordinal) {
    uint32_t comp_ordinal = std::numeric_limits<uint32_t>::max();

    for (uint32_t i = 0; i < props.size(); ++i) {
        if (props[i].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            comp_ordinal = i;
            break;
        }
    }

    if (comp_ordinal != std::numeric_limits<uint32_t>::max()) {
        *ordinal = comp_ordinal;
    }
    else {
        LOG_WARN("could not find queue ordinal, ordinal 0 will be used");
        *ordinal = 0;
    }
}

void get_queue_index(const ze_queue_properties_t& props,
                     uint32_t ordinal,
                     int rank,
                     uint32_t* index) {
    CCL_ASSERT(props.size() > ordinal, "props.size() <= ordinal");
    *index = rank % props[ordinal].numQueues;
}

std::string to_string(const ze_result_t result) {
    switch (result) {
        case ZE_RESULT_SUCCESS: return "ZE_RESULT_SUCCESS";
        case ZE_RESULT_NOT_READY: return "ZE_RESULT_NOT_READY";
        case ZE_RESULT_ERROR_DEVICE_LOST: return "ZE_RESULT_ERROR_DEVICE_LOST";
        case ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY: return "ZE_RESULT_ERROR_OUT_OF_HOST_MEMORY";
        case ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY: return "ZE_RESULT_ERROR_OUT_OF_DEVICE_MEMORY";
        case ZE_RESULT_ERROR_MODULE_BUILD_FAILURE: return "ZE_RESULT_ERROR_MODULE_BUILD_FAILURE";
        case ZE_RESULT_ERROR_MODULE_LINK_FAILURE: return "ZE_RESULT_ERROR_MODULE_LINK_FAILURE";
        case ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS:
            return "ZE_RESULT_ERROR_INSUFFICIENT_PERMISSIONS";
        case ZE_RESULT_ERROR_NOT_AVAILABLE: return "ZE_RESULT_ERROR_NOT_AVAILABLE";
        case ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE:
            return "ZE_RESULT_ERROR_DEPENDENCY_UNAVAILABLE";
        case ZE_RESULT_ERROR_UNINITIALIZED: return "ZE_RESULT_ERROR_UNINITIALIZED";
        case ZE_RESULT_ERROR_UNSUPPORTED_VERSION: return "ZE_RESULT_ERROR_UNSUPPORTED_VERSION";
        case ZE_RESULT_ERROR_UNSUPPORTED_FEATURE: return "ZE_RESULT_ERROR_UNSUPPORTED_FEATURE";
        case ZE_RESULT_ERROR_INVALID_ARGUMENT: return "ZE_RESULT_ERROR_INVALID_ARGUMENT";
        case ZE_RESULT_ERROR_INVALID_NULL_HANDLE: return "ZE_RESULT_ERROR_INVALID_NULL_HANDLE";
        case ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE: return "ZE_RESULT_ERROR_HANDLE_OBJECT_IN_USE";
        case ZE_RESULT_ERROR_INVALID_NULL_POINTER: return "ZE_RESULT_ERROR_INVALID_NULL_POINTER";
        case ZE_RESULT_ERROR_INVALID_SIZE: return "ZE_RESULT_ERROR_INVALID_SIZE";
        case ZE_RESULT_ERROR_UNSUPPORTED_SIZE: return "ZE_RESULT_ERROR_UNSUPPORTED_SIZE";
        case ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT: return "ZE_RESULT_ERROR_UNSUPPORTED_ALIGNMENT";
        case ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT:
            return "ZE_RESULT_ERROR_INVALID_SYNCHRONIZATION_OBJECT";
        case ZE_RESULT_ERROR_INVALID_ENUMERATION: return "ZE_RESULT_ERROR_INVALID_ENUMERATION";
        case ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION:
            return "ZE_RESULT_ERROR_UNSUPPORTED_ENUMERATION";
        case ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT:
            return "ZE_RESULT_ERROR_UNSUPPORTED_IMAGE_FORMAT";
        case ZE_RESULT_ERROR_INVALID_NATIVE_BINARY: return "ZE_RESULT_ERROR_INVALID_NATIVE_BINARY";
        case ZE_RESULT_ERROR_INVALID_GLOBAL_NAME: return "ZE_RESULT_ERROR_INVALID_GLOBAL_NAME";
        case ZE_RESULT_ERROR_INVALID_KERNEL_NAME: return "ZE_RESULT_ERROR_INVALID_KERNEL_NAME";
        case ZE_RESULT_ERROR_INVALID_FUNCTION_NAME: return "ZE_RESULT_ERROR_INVALID_FUNCTION_NAME";
        case ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION:
            return "ZE_RESULT_ERROR_INVALID_GROUP_SIZE_DIMENSION";
        case ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION:
            return "ZE_RESULT_ERROR_INVALID_GLOBAL_WIDTH_DIMENSION";
        case ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX:
            return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_INDEX";
        case ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE:
            return "ZE_RESULT_ERROR_INVALID_KERNEL_ARGUMENT_SIZE";
        case ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE:
            return "ZE_RESULT_ERROR_INVALID_KERNEL_ATTRIBUTE_VALUE";
        case ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED:
            return "ZE_RESULT_ERROR_INVALID_MODULE_UNLINKED";
        case ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE:
            return "ZE_RESULT_ERROR_INVALID_COMMAND_LIST_TYPE";
        case ZE_RESULT_ERROR_OVERLAPPING_REGIONS: return "ZE_RESULT_ERROR_OVERLAPPING_REGIONS";
        case ZE_RESULT_ERROR_UNKNOWN: return "ZE_RESULT_ERROR_UNKNOWN";
        case ZE_RESULT_FORCE_UINT32: return "ZE_RESULT_FORCE_UINT32";
        default: return "unknown ze_result_t value: " + std::to_string(static_cast<int>(result));
    }
}

std::string to_string(const ze_group_size_t& group_size) {
    std::stringstream ss;
    ss << "{ x: " << group_size.groupSizeX << ", y: " << group_size.groupSizeY
       << ", z: " << group_size.groupSizeZ << " }";
    return ss.str();
}

std::string to_string(const ze_group_count_t& group_count) {
    std::stringstream ss;
    ss << "{ x: " << group_count.groupCountX << ", y: " << group_count.groupCountY
       << ", z: " << group_count.groupCountZ << " }";
    return ss.str();
}

std::string to_string(const ze_kernel_args_t& kernel_args) {
    std::stringstream ss;
    ss << "{\n";
    size_t idx = 0;
    for (const auto& arg : kernel_args) {
        // TODO: can we distinguish argument types in order to properly print them instead of printing
        // as a void* ptr?
        ss << "  idx: " << idx << ", { " << arg.first << ", " << *(void**)arg.second << " }\n";
        ++idx;
    }
    ss << "}";
    return ss.str();
}

} // namespace ze
} // namespace ccl
