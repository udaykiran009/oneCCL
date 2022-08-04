#include <level_zero/ze_api.h>
#include <level_zero/zes_api.h>

#include <cassert>
#include <iomanip>
#include <iostream>
#include <string>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#include "options.hpp"

#define ZE_CALL(func, args) assert(func args == 0)

bool is_system_healthy = true;
bool is_root_priv = false;

std::vector<ze_driver_handle_t> drivers;
std::unordered_map<ze_driver_handle_t, std::vector<ze_device_handle_t>> devices;

std::string to_string(zes_fabric_port_status_t status) {
    switch (status) {
        case ZES_FABRIC_PORT_STATUS_UNKNOWN: return "unknown";
        case ZES_FABRIC_PORT_STATUS_HEALTHY: return "healthy";
        case ZES_FABRIC_PORT_STATUS_DEGRADED: return "degraded";
        case ZES_FABRIC_PORT_STATUS_FAILED: return "failed";
        case ZES_FABRIC_PORT_STATUS_DISABLED: return "disabled";
        default: return "unexpected";
    }
}

std::string to_string(zes_mem_health_t health) {
    switch (health) {
        case ZES_MEM_HEALTH_UNKNOWN: return "unknown";
        case ZES_MEM_HEALTH_OK: return "ok";
        case ZES_MEM_HEALTH_DEGRADED: return "degraded";
        case ZES_MEM_HEALTH_CRITICAL: return "critical";
        case ZES_MEM_HEALTH_REPLACE: return "replace";
        default: return "unexpected";
    }
}

std::string get_port_qual_issue_flags(zes_fabric_port_qual_issue_flags_t flags) {
    std::string str;
    if (flags & ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS) {
        str.append("ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_LINK_ERRORS ");
    }
    if (flags & ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED) {
        str.append("ZES_FABRIC_PORT_QUAL_ISSUE_FLAG_SPEED ");
    }
    return str;
}

std::string get_port_failure_flags(zes_fabric_port_failure_flags_t flags) {
    std::string str;
    if (flags & ZES_FABRIC_PORT_FAILURE_FLAG_FAILED) {
        str.append("ZES_FABRIC_PORT_FAILURE_FLAG_FAILED ");
    }
    if (flags & ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT) {
        str.append("ZES_FABRIC_PORT_FAILURE_FLAG_TRAINING_TIMEOUT ");
    }
    if (flags & ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING) {
        str.append("ZES_FABRIC_PORT_FAILURE_FLAG_FLAPPING ");
    }
    return str;
}

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

void print_device_uuid(ze_device_handle_t device) {
    ze_device_properties_t device_props = {
        .stype = ZE_STRUCTURE_TYPE_DEVICE_PROPERTIES,
        .pNext = nullptr,
    };
    ZE_CALL(zeDeviceGetProperties, (device, &device_props));
    std::cout << "Device UUID: ";
    for (auto id : device_props.uuid.id) {
        std::cout << std::to_string(id);
    }
    std::cout << std::endl;
}

void print_fabric_port_status() {
    std::cout << "---" << std::endl;
    std::cout << "Fabric port status:" << std::endl;
    std::cout << "---" << std::endl;

    bool fabric_ports_found = false;

    for (auto& driver_devices_pair : devices) {
        auto& device_list = driver_devices_pair.second;
        for (auto& device : device_list) {
            uint32_t count = 0;
            ZE_CALL(zesDeviceEnumFabricPorts, (device, &count, nullptr));
            std::vector<zes_fabric_port_handle_t> ports(count, nullptr);
            ZE_CALL(zesDeviceEnumFabricPorts, (device, &count, ports.data()));

            fabric_ports_found = !ports.empty();
            if (!fabric_ports_found) {
                continue;
            }

            print_device_uuid(device);
            for (auto& port : ports) {
                zes_fabric_port_properties_t props{};
                ZE_CALL(zesFabricPortGetProperties, (port, &props));

                zes_fabric_port_state_t state;
                ZE_CALL(zesFabricPortGetState, (port, &state));

                std::cout << "port ID: [" << props.portId.fabricId << ":" << props.portId.attachId
                          << ":" << static_cast<uint32_t>(props.portId.portNumber) << "]";

                std::cout << ", remote port ID: [" << state.remotePortId.fabricId << ":"
                          << state.remotePortId.attachId << ":"
                          << static_cast<uint32_t>(state.remotePortId.portNumber) << "]";

                zes_fabric_port_config_t config{};
                ZE_CALL(zesFabricPortGetConfig, (port, &config));
                std::cout << ", enabled: " << ((config.enabled) ? "up" : "down");
                std::cout << ", consistent: "
                          << ((props.portId.attachId == state.remotePortId.attachId) ? "true "
                                                                                     : "false");

                zes_fabric_port_status_t status = state.status;
                std::cout << ", status: " << to_string(status);
                if (status == ZES_FABRIC_PORT_STATUS_DEGRADED) {
                    std::cout << ", degraded reason: "
                              << get_port_qual_issue_flags(state.qualityIssues);
                }
                if (status == ZES_FABRIC_PORT_STATUS_FAILED) {
                    std::cout << ", failed reason: "
                              << get_port_failure_flags(state.failureReasons);
                }
                if (status != ZES_FABRIC_PORT_STATUS_HEALTHY) {
                    is_system_healthy = false;
                }

                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }

    if (!fabric_ports_found) {
        std::cout << "No fabric ports\n" << std::endl;
    }
}

void print_memory_status() {
    std::cout << "---" << std::endl;
    std::cout << "Memory status:" << std::endl;
    std::cout << "---" << std::endl;
    for (auto& driver_devices_pair : devices) {
        auto& device_list = driver_devices_pair.second;
        for (auto& device : device_list) {
            print_device_uuid(device);

            uint32_t count = 0;
            ZE_CALL(zesDeviceEnumMemoryModules, (device, &count, nullptr));
            if (count == 0) {
                std::cout << "could not retrieve Firmware domains" << std::endl;
                continue;
            }
            std::vector<zes_mem_handle_t> memory_handles(count, nullptr);
            ZE_CALL(zesDeviceEnumMemoryModules, (device, &count, memory_handles.data()));

            for (auto handle : memory_handles) {
                zes_mem_state_t state{};
                ZE_CALL(zesMemoryGetState, (handle, &state));
                std::cout << "health: " << to_string(state.health);
                if (state.health != ZES_MEM_HEALTH_OK) {
                    is_system_healthy = false;
                }
                std::cout << ", free: " << human_bytes(state.free);
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }
}

void print_firmwares() {
    std::cout << "---" << std::endl;
    std::cout << "Firmwares:" << std::endl;
    if (!is_root_priv) {
        std::cout << "Root privileges may be required to obtain this information" << std::endl;
    }
    std::cout << "---" << std::endl;
    for (auto& driver_devices_pair : devices) {
        auto& device_list = driver_devices_pair.second;
        for (auto& device : device_list) {
            print_device_uuid(device);

            uint32_t count = 0;
            ZE_CALL(zesDeviceEnumFirmwares, (device, &count, nullptr));
            if (count == 0) {
                std::cout << "could not retrieve Firmware domains" << std::endl;
                continue;
            }
            std::vector<zes_firmware_handle_t> firmwares(count, nullptr);
            ZE_CALL(zesDeviceEnumFirmwares, (device, &count, firmwares.data()));

            for (auto firmware : firmwares) {
                zes_firmware_properties_t props{};
                ZE_CALL(zesFirmwareGetProperties, (firmware, &props));
                std::cout << "name: " << props.name;
                std::cout << ", version: " << props.version;
                std::cout << std::endl;
            }
            std::cout << std::endl;
        }
    }
}

int main(int argc, char* argv[]) {
    setenv("ZES_ENABLE_SYSMAN", "1", 1);

    parse_options(argc, argv);

    if (!getuid()) {
        is_root_priv = true;
    }

    ZE_CALL(zeInit, (0));

    uint32_t driver_count = 0;
    ZE_CALL(zeDriverGet, (&driver_count, nullptr));
    drivers.resize(driver_count);
    ZE_CALL(zeDriverGet, (&driver_count, drivers.data()));

    for (auto& driver : drivers) {
        uint32_t device_count = 0;
        ZE_CALL(zeDeviceGet, (driver, &device_count, nullptr));
        auto& device_list = devices[driver];
        device_list.resize(device_count);
        ZE_CALL(zeDeviceGet, (driver, &device_count, device_list.data()));
    }

    if (options.show_all || options.show_links_only) {
        print_fabric_port_status();
    }
    if (options.show_all || options.show_mem_only) {
        print_memory_status();
    }
    if (options.show_all || options.show_firmware_only) {
        print_firmwares();
    }

    return !is_system_healthy;
}
