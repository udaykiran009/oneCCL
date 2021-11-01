#include <algorithm>
#include <fstream>

#include "common/global/global.hpp"
#include "common/log/log.hpp"
#include "sched/entry/ze/ze_primitives.hpp"

namespace ccl {

namespace ze {

std::string get_build_log_string(ze_module_build_log_handle_t build_log) {
    size_t log_size{};
    ZE_CALL(zeModuleBuildLogGetString, (build_log, &log_size, nullptr));

    if (!log_size) {
        LOG_DEBUG(log_size, "empty build log");
        return {};
    }

    std::string log(log_size, '\0');
    ZE_CALL(zeModuleBuildLogGetString, (build_log, &log_size, const_cast<char*>(log.data())));
    return log;
}

void load_module(const std::string& file_path,
                 ze_device_handle_t device,
                 ze_context_handle_t context,
                 ze_module_handle_t* module) {
    LOG_DEBUG("module loading started: file: ", file_path);
    CCL_THROW_IF_NOT(!file_path.empty(), "no file");

    std::ifstream file(file_path, std::ios_base::in | std::ios_base::binary);
    CCL_THROW_IF_NOT(file.good(), "failed to load module: file: ", file_path);

    file.seekg(0, file.end);
    size_t filesize = file.tellg();
    file.seekg(0, file.beg);

    std::vector<uint8_t> module_data(filesize);
    file.read(reinterpret_cast<char*>(module_data.data()), filesize);
    file.close();

    ze_module_build_log_handle_t build_log{};
    ze_module_desc_t desc{};
    ze_module_format_t format = ZE_MODULE_FORMAT_IL_SPIRV;
    desc.format = format;
    desc.pInputModule = reinterpret_cast<const uint8_t*>(module_data.data());
    desc.inputSize = module_data.size();

    if (zeModuleCreate(context, device, &desc, module, &build_log) != ZE_RESULT_SUCCESS) {
        CCL_THROW(
            "failed to create module: ", file_path, ", log: ", get_build_log_string(build_log));
    }
    else {
        LOG_DEBUG("module loading completed: directory: file: ", file_path);
    }

    zeModuleBuildLogDestroy(build_log);
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
                              size_t elem_count,
                              ze_group_size_t* group_size) {
    group_size->groupSizeX = group_size->groupSizeY = group_size->groupSizeZ = 1;
    if (!elem_count) {
        return;
    }

    ZE_CALL(zeKernelSuggestGroupSize,
            (kernel,
             elem_count,
             1,
             1,
             &group_size->groupSizeX,
             &group_size->groupSizeY,
             &group_size->groupSizeZ));

    CCL_THROW_IF_NOT(group_size->groupSizeX >= 1,
                     "wrong group size calculation: size: ",
                     to_string(*group_size),
                     ", elem_count: ",
                     elem_count);
}

void get_suggested_group_count(const ze_group_size_t& group_size,
                               size_t elem_count,
                               ze_group_count_t* group_count) {
    group_count->groupCountX = std::max((elem_count / group_size.groupSizeX), 1ul);
    group_count->groupCountY = 1;
    group_count->groupCountZ = 1;

    auto rem = elem_count % group_size.groupSizeX;
    CCL_THROW_IF_NOT(group_count->groupCountX >= 1 && rem == 0,
                     "wrong group calculation: size: ",
                     to_string(group_size),
                     ", count: ",
                     to_string(*group_count),
                     ", elem_count: ",
                     std::to_string(elem_count));
}

void set_kernel_args(ze_kernel_handle_t kernel, const ze_kernel_args_t& kernel_args) {
    uint32_t idx = 0;
    for (const auto& arg : kernel_args) {
        auto res = zeKernelSetArgumentValue(kernel, idx, arg.size, arg.ptr);
        if (res != ZE_RESULT_SUCCESS) {
            CCL_THROW("zeKernelSetArgumentValue failed with error ",
                      to_string(res),
                      " on idx ",
                      idx,
                      " with value ",
                      *((void**)arg.ptr));
        }
        ++idx;
    }
}

void get_queues_properties(ze_device_handle_t device, ze_queue_properties_t* props) {
    uint32_t queue_group_count = 0;
    ZE_CALL(zeDeviceGetCommandQueueGroupProperties, (device, &queue_group_count, nullptr));

    CCL_THROW_IF_NOT(queue_group_count != 0, "no queue groups found");

    props->resize(queue_group_count);
    ZE_CALL(zeDeviceGetCommandQueueGroupProperties, (device, &queue_group_count, props->data()));
}

void get_comp_queue_ordinal(const ze_queue_properties_t& props, uint32_t* ordinal) {
    CCL_THROW_IF_NOT(!props.empty(), "no queue props");
    uint32_t comp_ordinal = std::numeric_limits<uint32_t>::max();

    for (uint32_t idx = 0; idx < props.size(); ++idx) {
        if (props[idx].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) {
            comp_ordinal = idx;
            break;
        }
    }

    if (comp_ordinal != std::numeric_limits<uint32_t>::max()) {
        LOG_DEBUG("find queue: { ordinal: ",
                  comp_ordinal,
                  ", queue properties params: ",
                  to_string(props[comp_ordinal]),
                  " }");
        *ordinal = comp_ordinal;
    }
    else {
        LOG_WARN("could not find queue ordinal, ordinal 0 will be used. Total queue group count: ",
                 props.size());
        *ordinal = 0;
    }
}

void get_copy_queue_ordinal(const ze_queue_properties_t& props, uint32_t* ordinal) {
    CCL_THROW_IF_NOT(!props.empty(), "no queue props");
    uint32_t copy_ordinal = std::numeric_limits<uint32_t>::max();

    for (uint32_t idx = 0; idx < props.size(); ++idx) {
        /* only compute property */
        if ((props[idx].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) &&
            global_data::env().ze_copy_engine == ccl_ze_copy_engine_none) {
            copy_ordinal = idx;
            break;
        }

        /* only copy property */
        if ((props[idx].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY) &&
            ((props[idx].flags & ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE) == 0)) {
            /* main */
            if (props[idx].numQueues == 1 &&
                global_data::env().ze_copy_engine == ccl_ze_copy_engine_main) {
                copy_ordinal = idx;
                break;
            }
            /* link */
            if (props[idx].numQueues > 1 &&
                global_data::env().ze_copy_engine == ccl_ze_copy_engine_link) {
                copy_ordinal = idx;
                break;
            }
        }
    }

    if (copy_ordinal != std::numeric_limits<uint32_t>::max()) {
        LOG_DEBUG("find copy queue: { ordinal: ",
                  copy_ordinal,
                  ", queue properties params: ",
                  to_string(props[copy_ordinal]),
                  " }");
        *ordinal = copy_ordinal;
    }
    else {
        LOG_WARN("could not find queue ordinal for copy engine mode: ",
                 global_data::env().ze_copy_engine,
                 ", ordinal 0 will be used. Total queue group count: ",
                 props.size());
        *ordinal = 0;
    }
}

void get_queue_index(const ze_queue_properties_t& props,
                     uint32_t ordinal,
                     int idx,
                     uint32_t* index) {
    CCL_ASSERT(props.size() > ordinal, "props.size() <= ordinal");

    idx += ccl::global_data::env().ze_queue_index;

    *index = idx % props[ordinal].numQueues;
    LOG_DEBUG("set queue index: ", *index);
}

device_family get_device_family(ze_device_handle_t device) {
    ze_device_properties_t device_prop;
    ZE_CALL(zeDeviceGetProperties, (device, &device_prop));
    uint32_t id = device_prop.deviceId & 0xfff0;
    using enum_t = typename std::underlying_type<device_family>::type;

    switch (id) {
        case static_cast<enum_t>(device_id::id1): return device_family::family1;
        case static_cast<enum_t>(device_id::id2): return device_family::family2;
        default: return device_family::unknown;
    }
}

// adjust the timestamp to a common format
static uint64_t adjust_device_timestamp(uint64_t timestamp, const ze_device_properties_t& props) {
    // we have 2 fields that specify the amount of value bits: kernelTimestampValidBits for event
    // timestamps and timestampValidBits for the global timestamps. In order to compare timestamps
    // from the different sources we need to truncate the value the lowest number of bits.
    const uint64_t min_mask = std::min(props.kernelTimestampValidBits, props.timestampValidBits);
    const uint64_t mask = (1ull << min_mask) - 1ull;

    return (timestamp & mask) * props.timerResolution;
}

// Returns start and end values for the provided event(measured in ns)
std::pair<uint64_t, uint64_t> calculate_event_time(ze_event_handle_t event,
                                                   ze_device_handle_t device) {
    ze_kernel_timestamp_result_t timestamp = {};
    ZE_CALL(zeEventQueryKernelTimestamp, (event, &timestamp));

    ze_device_properties_t device_props = {};
    device_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    ZE_CALL(zeDeviceGetProperties, (device, &device_props));

    // use global counter as we calculate value across different contexts
    uint64_t start = timestamp.global.kernelStart;
    uint64_t end = timestamp.global.kernelEnd;

    // gpu counters might be limited to 32-bit, so we need to handle a potential overlap
    if (end <= start) {
        const uint64_t timestamp_max_value = (1LL << device_props.kernelTimestampValidBits) - 1;
        end += timestamp_max_value - start;
    }

    start = adjust_device_timestamp(start, device_props);
    end = adjust_device_timestamp(end, device_props);

    return { start, end };
}

uint64_t calculate_global_time(ze_device_handle_t device) {
    uint64_t host_timestamp = 0;
    uint64_t device_timestamp = 0;
    ZE_CALL(zeDeviceGetGlobalTimestamps, (device, &host_timestamp, &device_timestamp));

    ze_device_properties_t device_props = {};
    device_props.stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES;
    ZE_CALL(zeDeviceGetProperties, (device, &device_props));

    return adjust_device_timestamp(device_timestamp, device_props);
}

std::string to_string(ze_result_t result) {
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
        ss << "  idx: " << idx << ", { " << arg.size << ", " << *(void**)arg.ptr << " }\n";
        ++idx;
    }
    ss << "}";
    return ss.str();
}

std::string to_string(const ze_command_queue_group_property_flag_t& flag) {
    switch (flag) {
        case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE:
            return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COMPUTE";
        case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY:
            return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COPY";
        case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS:
            return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_COOPERATIVE_KERNELS";
        case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS:
            return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_METRICS";
        case ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_FORCE_UINT32:
            return "ZE_COMMAND_QUEUE_GROUP_PROPERTY_FLAG_FORCE_UINT32";
        default:
            return "unknown ze_command_queue_group_property_flag_t value: " +
                   std::to_string(static_cast<int>(flag));
    }
}

template <typename T>
std::string flags_to_string(uint32_t flags) {
    constexpr size_t bits = 8;
    std::vector<std::string> output;
    for (size_t i = 0; i < sizeof(flags) * bits; ++i) {
        const size_t mask = 1UL << i;
        const auto flag = flags & mask;
        if (flag != 0) {
            output.emplace_back(to_string(static_cast<T>(flag)));
        }
    }
    return join_strings(output, " | ");
}

std::string to_string(const ze_command_queue_group_properties_t& queue_property) {
    std::stringstream ss;
    ss << "stype: " << queue_property.stype << ", pNext: " << (void*)queue_property.pNext
       << ", flags: "
       << flags_to_string<ze_command_queue_group_property_flag_t>(queue_property.flags)
       << ", maxMemoryFillPatternSize: " << queue_property.maxMemoryFillPatternSize
       << ", numQueues: " << queue_property.numQueues;
    return ss.str();
}

std::string join_strings(const std::vector<std::string>& tokens, const std::string& delimeter) {
    std::stringstream ss;
    for (size_t i = 0; i < tokens.size(); ++i) {
        ss << tokens[i];
        if (i < tokens.size() - 1) {
            ss << delimeter;
        }
    }
    return ss.str();
}

} // namespace ze
} // namespace ccl
