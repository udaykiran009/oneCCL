#pragma once
#include <memory>
#include <string>
#include <type_traits>
#include "common/utils/enums.hpp"

namespace native
{
enum class gpu_types : size_t
{
    REAL_GPU,
    VIRTUAL_GPU,
    CONCURRENT_GPU,
    CONCURRENT_REAL_GPU = CONCURRENT_GPU,
    CONCURRENT_VIRT_GPU,
    IPC_GPU,
    IPC_SOURCE_REAL_GPU = IPC_GPU,
    IPC_SOURCE_VIRT_GPU,
    IPC_DESTINATION_GPU,
    SCALE_UP_GPU,
    SCALE_UP_REAL_GPU = SCALE_UP_GPU,
    SCALE_UP_VIRTUAL_GPU,
    MAX_TYPE
};

using gpu_type_names = utils::enum_to_str<static_cast<int>(gpu_types::MAX_TYPE)>;
inline std::string to_string(gpu_types type)
{
    return gpu_type_names({"REAL_GPU", "VIRTUAL_GPU",
                           "CONCURRENT_REAL_GPU", "CONCURRENT_VIRT_GPU",
                           "SOURCE_IPC_REAL_GPU", "SOURCE_IPC_VIRT_GPU",
                           "DESTINATION_IPC_GPU",
                           "SCALE_UP_REAL_GPU", "SCALE_UP_VIRTUAL_GPU"}).choose(type, "INVALID_VALUE");
}

constexpr inline gpu_types operator+ (gpu_types a, typename std::underlying_type<gpu_types>::type b)
{
    return static_cast<gpu_types>(static_cast< typename std::underlying_type<gpu_types>::type>(a)
           + static_cast< typename std::underlying_type<gpu_types>::type>(b));
}

// devices
template<class device_t>
using device_t_ptr = std::shared_ptr<device_t>;
}
