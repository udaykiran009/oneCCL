#pragma once

#include "ccl_type_traits.hpp"
#include "common/log/log.hpp"

#ifdef CCL_ENABLE_SYCL
CCL_API generic_device_type<CCL_ENABLE_SYCL_TRUE>::generic_device_type(device_index_type id,
                                                                       cl::sycl::info::device_type type/* = info::device_type::gpu*/)
    :device()
{
    LOG_DEBUG("Try to find SYCL device by index: ", id, ", type: ",
              static_cast<typename std::underlying_type<cl::sycl::info::device_type>::type>(type));

    auto platforms = cl::sycl::platform::get_platforms();
    LOG_DEBUG("Found CL plalforms: ", platforms.size());
    auto platform_it =
            std::find_if(platforms.begin(), platforms.end(),
                        [](const cl::sycl::platform& pl)
                        {
                            return pl.get_info<cl::sycl::info::platform::name>().find("Level-Zero") != std::string::npos;
                            //or platform.get_backend() == cl::sycl::backend::level0
                        });
    if (platform_it == platforms.end())
    {
        std::stringstream ss;
        ss << "cannot find Level-Zero platform. Supported platforms are:\n";
        for (const auto& pl : platforms)
        {
            ss << "Platform:\nprofile: " << pl.get_info<cl::sycl::info::platform::profile>()
               << "\nversion: " << pl.get_info<cl::sycl::info::platform::version>()
               << "\nname: " << pl.get_info<cl::sycl::info::platform::name>()
               << "\nvendor: " << pl.get_info<cl::sycl::info::platform::vendor>();
        }

        throw std::runtime_error(std::string("Cannot find device by id: ") + ccl::to_string(id) +
                                 ", reason:\n" + ss.str());
    }

    LOG_DEBUG("Platform:\nprofile: ", platform_it->get_info<cl::sycl::info::platform::profile>(),
              "\nversion: ", platform_it->get_info<cl::sycl::info::platform::version>(),
              "\nname: ", platform_it->get_info<cl::sycl::info::platform::name>(),
              "\nvendor: ", platform_it->get_info<cl::sycl::info::platform::vendor>());
    auto devices = platform_it->get_devices(type);

    LOG_DEBUG("Found devices: ", devices.size());
    auto it = std::find_if(devices.begin(), devices.end(), [id] (const cl::sycl::device& dev)
    {
        return id == native::get_runtime_device(dev)->get_device_path();
    });
    if(it == devices.end())
    {
        std::stringstream ss;
        ss << "cannot find requested device. Supported devices are:\n";
        for (const auto& dev : devices)
        {
            ss << "Device:\nname: " << dev.get_info<cl::sycl::info::device::name>()
               << "\nvendor: " << dev.get_info<cl::sycl::info::device::vendor>()
               << "\nversion: " << dev.get_info<cl::sycl::info::device::version>()
               << "\nprofile: " << dev.get_info<cl::sycl::info::device::profile>();
        }

        throw std::runtime_error(std::string("Cannot find device by id: ") + ccl::to_string(id) +
                                 ", reason:\n" + ss.str());
    }
    device = *it;
}

generic_device_type<CCL_ENABLE_SYCL_TRUE>::generic_device_type(const cl::sycl::device &in_device)
     :device(in_device)
{
}

device_index_type generic_device_type<CCL_ENABLE_SYCL_TRUE>::get_id() const noexcept
{
    return native::get_runtime_device(device)->get_device_path();
}

typename generic_device_type<CCL_ENABLE_SYCL_TRUE>::native_reference_t
generic_device_type<CCL_ENABLE_SYCL_TRUE>::get() noexcept
{
    return device;
}

#else

generic_device_type<CCL_ENABLE_SYCL_FALSE>::generic_device_type(device_index_type id)
 :device(id)
{
}

device_index_type generic_device_type<CCL_ENABLE_SYCL_FALSE>::get_id() const noexcept
{
    return device;
}

typename generic_device_type<CCL_ENABLE_SYCL_FALSE>::native_reference_t
generic_device_type<CCL_ENABLE_SYCL_FALSE>::get() noexcept
{
    return native::get_runtime_device(device);
}
#endif
