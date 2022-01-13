#include "common/global/global.hpp"
#include "common/topology/topo_manager.hpp"

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include "common/utils/sycl_utils.hpp"
#include "sched/entry/ze/ze_primitives.hpp"
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

namespace ccl {

rank_topo_info::rank_topo_info() : rank(ccl_comm::invalid_rank) {}

constexpr int topo_manager::invalid_color;

void topo_manager::init(std::shared_ptr<atl_base_comm> atl_comm,
                        std::shared_ptr<ccl::device> device_ptr,
                        std::shared_ptr<ccl::context> context_ptr) {
    int comm_rank = atl_comm->get_rank();
    int comm_size = atl_comm->get_size();

    intra_card_colors.resize(comm_size, topo_manager::invalid_color);
    inter_card_colors.resize(comm_size, topo_manager::invalid_color);
    p2p_matrix.resize(comm_size);
    for (size_t i = 0; i < p2p_matrix.size(); i++) {
        p2p_matrix[i].resize(comm_size, true);
    }

    std::vector<char> all_hostnames(max_hostname_len * comm_size);
    char my_hostname[max_hostname_len] = { 0 };
    gethostname(my_hostname, max_hostname_len - 1);

    LOG_DEBUG("rank: ", comm_rank, ", size: ", comm_size, ", hostname: ", my_hostname);
    int recv_byte = max_hostname_len;
    std::vector<int> recv_bytes(comm_size, recv_byte);
    std::vector<int> offsets(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
    }

    atl_req_t req{};
    atl_comm->allgatherv(0 /* ep_idx */,
                         my_hostname,
                         max_hostname_len,
                         all_hostnames.data(),
                         recv_bytes.data(),
                         offsets.data(),
                         &req);
    atl_comm->wait(0 /* ep_idx */, &req);

    std::set<std::string> unique_hostnames;
    std::string str{};
    for (int idx = 0; idx < comm_size; idx++) {
        str = std::string(all_hostnames.data() + (idx * max_hostname_len), max_hostname_len);
        unique_hostnames.insert(str);
    }

    int local_idx = 0;
    for (auto host_name : unique_hostnames) {
        if (strcmp(my_hostname, host_name.c_str()) == 0) {
            host_idx = local_idx;
            break;
        }
        else {
            local_idx++;
        }
    }

    CCL_THROW_IF_NOT(host_idx != topo_manager::invalid_host_idx,
                     "invalid host index for topology building, hostname: ",
                     my_hostname);

    if (ccl::global_data::env().topo_color != topo_color_mode::ze)
        return;

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    if (!(device_ptr && context_ptr))
        return;

    rank_topo_info rank_info{};
    std::vector<rank_topo_info> topo_info_vec(comm_size);

    device = sycl::get_native<ccl::utils::get_level_zero_backend()>(device_ptr.get()->get_native());
    zes_device_handle_t zes_device = (zes_device_handle_t)device;

    rank_info.rank = comm_rank;
    rank_info.host_idx = host_idx;

    ze_device_properties_t dev_props = ccl::ze::default_device_props;
    ZE_CALL(zeDeviceGetProperties, (device, &dev_props));
    rank_info.uuid = dev_props.uuid;
    LOG_DEBUG("hostname: ",
              my_hostname,
              ", rank: ",
              comm_rank,
              ", size: ",
              comm_size,
              ", device uuid: ",
              ccl::ze::to_string(rank_info.uuid));

    zes_pci_properties_t pci_props = {};
    ZE_CALL(zesDevicePciGetProperties, (device, &pci_props));
    rank_info.pci_addr = pci_props.address;

    recv_bytes.clear();
    offsets.clear();
    recv_bytes.resize(comm_size, sizeof(rank_info));
    offsets.resize(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
    }
    atl_comm->allgatherv(0 /* ep_idx */,
                         &rank_info,
                         sizeof(rank_info),
                         reinterpret_cast<char*>(topo_info_vec.data()),
                         recv_bytes.data(),
                         offsets.data(),
                         &req);
    atl_comm->wait(0 /* ep_idx */, &req);

    // create groups with its unique color

    int topo_info_size = static_cast<int>(topo_info_vec.size());
    for (int i = 0; i < topo_info_size; i++) {
        // skip the marked rank
        if (intra_card_colors[i] != topo_manager::invalid_color)
            continue;

        if (topo_info_vec[i].host_idx != rank_info.host_idx)
            continue;

        intra_card_colors[i] = i; // create new group with color

        for (int j = i + 1; j <= topo_info_size; j++) {
            // my rank and right neighboor have the same pci address?
            if (is_same_pci_addr(topo_info_vec[i].pci_addr, topo_info_vec[j].pci_addr)) {
                intra_card_colors[j] = i;
            }
        }
    }

    uint32_t subdev_count{};
    ZE_CALL(zeDeviceGetSubDevices, (device, &subdev_count, nullptr));
    LOG_INFO("(",
             my_hostname,
             ") rank ",
             comm_rank,
             " has ",
             subdev_count,
             " subdevices, ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE=",
             (int)(dev_props.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE));

    if (dev_props.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE) {
        LOG_INFO("(", my_hostname, ") rank ", comm_rank, " subdeviceId ", dev_props.subdeviceId);
    }

    uint32_t port_count{};
    ZE_CALL(zesDeviceEnumFabricPorts, (zes_device, &port_count, NULL));
    std::vector<zes_fabric_port_handle_t> ports(port_count);
    ZE_CALL(zesDeviceEnumFabricPorts, (zes_device, &port_count, ports.data()));

    char* all_ports_env = getenv("CCL_TOPO_ALL_PORTS"); //To be deleted in future

    std::vector<port_topo_info> my_ports;
    for (size_t i = 0; i < ports.size(); i++) {
        zes_fabric_port_properties_t prop;
        ZE_CALL(zesFabricPortGetProperties, (ports[i], &prop));

        zes_fabric_port_config_t config;
        ZE_CALL(zesFabricPortGetConfig, (ports[i], &config));

        if (!config.enabled) {
            LOG_INFO("(",
                     my_hostname,
                     ") rank ",
                     comm_rank,
                     " has disabled port ",
                     ccl::ze::to_string(prop.portId),
                     " skip it");
            continue;
        }

        zes_fabric_port_state_t state;
        ZE_CALL(zesFabricPortGetState, (ports[i], &state));

        if (state.status == ZES_FABRIC_PORT_STATUS_HEALTHY ||
            state.status == ZES_FABRIC_PORT_STATUS_DEGRADED) {
            // port has good status meaning it's connected

            if (all_ports_env && atoi(all_ports_env)) {
                my_ports.push_back({ prop.portId, state.remotePortId });
            }
            else {
                if (prop.onSubdevice && prop.subdeviceId == dev_props.subdeviceId)
                    my_ports.push_back({ prop.portId, state.remotePortId });
            }

            if (state.status == ZES_FABRIC_PORT_STATUS_DEGRADED) {
                LOG_INFO("(",
                         my_hostname,
                         ") rank ",
                         comm_rank,
                         " has a degraded port ",
                         ccl::ze::to_string(prop.portId));
            }
        }
    }

    // gather ports counts from every rank
    std::vector<int> all_ports_counts(comm_size);
    recv_bytes.clear();
    offsets.clear();
    recv_bytes.resize(comm_size, sizeof(int));
    offsets.resize(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
    }
    int my_ports_count = (int)my_ports.size();
    atl_comm->allgatherv(0,
                         &my_ports_count,
                         sizeof(int),
                         reinterpret_cast<char*>(all_ports_counts.data()),
                         recv_bytes.data(),
                         offsets.data(),
                         &req);
    atl_comm->wait(0, &req);

    // gather ports info from all the ranks
    recv_bytes.clear();
    offsets.clear();
    recv_bytes.resize(comm_size, sizeof(port_topo_info) * all_ports_counts[0]);
    offsets.resize(comm_size, 0);
    std::vector<int> port_count_offsets(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        recv_bytes[i] = sizeof(port_topo_info) * all_ports_counts[i];
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
        port_count_offsets[i] = port_count_offsets[i - 1] + all_ports_counts[i - 1];
    }

    std::vector<port_topo_info> all_ports(
        std::accumulate(all_ports_counts.begin(), all_ports_counts.end(), 0));
    atl_comm->allgatherv(0 /* ep_idx */,
                         reinterpret_cast<char*>(my_ports.data()),
                         sizeof(port_topo_info) * my_ports.size(),
                         reinterpret_cast<char*>(all_ports.data()),
                         recv_bytes.data(),
                         offsets.data(),
                         &req);
    atl_comm->wait(0 /* ep_idx */, &req);

    if (!comm_rank) {
        for (int rank_idx = 0; rank_idx < comm_size; rank_idx++) {
            LOG_INFO("--- rank ", topo_info_vec[rank_idx].rank, " ---");
            for (int port_idx = port_count_offsets[rank_idx];
                 port_idx < port_count_offsets[rank_idx] + all_ports_counts[rank_idx];
                 port_idx++) {
                LOG_INFO("port local ",
                         ccl::ze::to_string(all_ports[port_idx].local),
                         ", remote ",
                         ccl::ze::to_string(all_ports[port_idx].remote));
            }
        }
    }

    std::vector<std::set<int>> planes;
    for (int rank = 0; rank < comm_size; rank++) {
        int cur_plane_idx = topo_manager::invalid_plane_idx;
        for (size_t plane_idx = 0; plane_idx < planes.size(); plane_idx++) {
            if (planes[plane_idx].find(rank) != planes[plane_idx].end()) {
                cur_plane_idx = plane_idx;
                break;
            }
        }

        if (cur_plane_idx == topo_manager::invalid_plane_idx) {
            planes.push_back({ rank });
            cur_plane_idx = planes.size() - 1;
        }

        for (int my_port_idx = port_count_offsets[rank];
             my_port_idx < port_count_offsets[rank] + all_ports_counts[rank];
             my_port_idx++) {
            auto remote_port = all_ports[my_port_idx].remote;
            int peer_port_idx = 0;
            while (peer_port_idx < (int)all_ports.size()) {
                if (peer_port_idx == my_port_idx) {
                    peer_port_idx += all_ports_counts[rank]; //skipping my ports
                }
                else {
                    auto peer_port = all_ports[peer_port_idx].local;
                    if (peer_port.fabricId == remote_port.fabricId &&
                        peer_port.attachId == remote_port.attachId &&
                        (int)peer_port.portNumber == (int)remote_port.portNumber) {
                        int peer_idx = ccl_comm::invalid_rank;
                        for (int peer_rank = 0; peer_rank < comm_size; peer_rank++) {
                            if (peer_port_idx >= port_count_offsets[peer_rank]) {
                                if (peer_rank == comm_size - 1 ||
                                    peer_port_idx < port_count_offsets[peer_rank + 1]) {
                                    peer_idx = peer_rank;
                                    break;
                                }
                            }
                        }
                        CCL_THROW_IF_NOT(peer_idx != ccl_comm::invalid_rank && peer_idx >= 0 &&
                                             peer_idx < comm_size,
                                         "peer_idx=",
                                         peer_idx,
                                         "is not valid");

                        planes[cur_plane_idx].insert(topo_info_vec[peer_idx].rank);
                        break;
                    }
                    peer_port_idx++;
                }
            }
        }
    }

    for (int rank = 0; rank < comm_size; rank++) {
        for (int plane_idx = 0; plane_idx < (int)planes.size(); plane_idx++) {
            if (planes[plane_idx].find(rank) != planes[plane_idx].end()) {
                inter_card_colors[rank] = plane_idx;
                break;
            }
        }
    }

    if (!comm_rank) {
        for (int plane_idx = 0; plane_idx < (int)planes.size(); plane_idx++) {
            std::stringstream ss;
            std::copy(planes[plane_idx].begin(),
                      planes[plane_idx].end(),
                      std::ostream_iterator<int>(ss, ", "));
            LOG_INFO("plane ", plane_idx, " contains ranks: ", ss.str());
        }
        LOG_INFO(topo_manager::to_string());
    }

    CCL_THROW_IF_NOT(intra_card_colors[comm_rank] != topo_manager::invalid_color,
                     "intra_card_color for rank ",
                     comm_rank,
                     " is not valid");
    CCL_THROW_IF_NOT(std::find(inter_card_colors.begin(),
                               inter_card_colors.end(),
                               topo_manager::invalid_color) == inter_card_colors.end(),
                     "inter_card_color data is not valid");

    auto devices = global_data::get().ze_data->device_list;
    p2p_matrix = build_p2p_matrix(devices);
    LOG_DEBUG("p2p matrix: \n", to_string(p2p_matrix), "\nnumber of devices: ", devices.size());
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
}

int topo_manager::get_intra_card_color(int rank) {
    int color{};
    if (ccl::global_data::env().topo_color == topo_color_mode::ze) {
        color = intra_card_colors[rank];
    }
    else if (ccl::global_data::env().topo_color == topo_color_mode::fixed) {
        color = rank / 2;
    }
    color = color + host_idx * max_ranks_per_host;
    return color;
}

int topo_manager::get_inter_card_color(int rank) {
    int color{};
    if (ccl::global_data::env().topo_color == topo_color_mode::ze) {
        color = inter_card_colors[rank];
    }
    else if (ccl::global_data::env().topo_color == topo_color_mode::fixed) {
        color = rank % 2;
    }
    color = color + host_idx * max_ranks_per_host;
    return color;
}

bool topo_manager::has_p2p_access() const {
    bool has_access = true;
    for (size_t i = 0; i < p2p_matrix.size(); i++) {
        for (size_t j = 0; j < p2p_matrix[i].size(); j++) {
            if (!p2p_matrix[i][j]) {
                has_access = false;
                break;
            }
        }
    }
    LOG_DEBUG("p2p access status: ", has_access);
    return has_access;
}

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
bool topo_manager::is_same_pci_addr(zes_pci_address_t addr1, zes_pci_address_t addr2) {
    bool result = true;
    if (!(addr1.domain == addr2.domain && addr1.bus == addr2.bus && addr1.device == addr2.device &&
          addr1.function == addr2.function)) {
        result = false;
        LOG_DEBUG("pci addresses are not the same:"
                  "pci_addr_1: ",
                  addr1.domain,
                  addr1.bus,
                  addr1.device,
                  addr1.function,
                  "\npci_addr_2: ",
                  addr2.domain,
                  addr2.bus,
                  addr2.device,
                  addr2.function);
    }

    return result;
}

std::vector<std::vector<bool>> topo_manager::build_p2p_matrix(
    const std::vector<ze_device_handle_t>& devices) {
    size_t device_count = devices.size();
    std::vector<std::vector<bool>> matrix(device_count);

    for (uint32_t i = 0; i < device_count; i++) {
        matrix[i].resize(device_count);

        for (uint32_t j = 0; j < device_count; j++) {
            if (i == j) {
                matrix[i][j] = true;
            }
            else {
                ze_bool_t val;
                ZE_CALL(zeDeviceCanAccessPeer, (devices[i], devices[j], &val));
                matrix[i][j] = static_cast<bool>(val);
            }
        }
    }
    return matrix;
}

std::string topo_manager::to_string(const std::vector<std::vector<bool>>& matrix) {
    uint32_t device_count = matrix.size();
    std::stringstream ss;
    for (uint32_t j = 0; j < device_count; j++) {
        if (j > 9) {
            ss << "  " << j;
        }
        else {
            ss << "   " << j;
        }
    }
    ss << "\n";

    for (uint32_t i = 0; i < device_count; i++) {
        if (i > 9) {
            ss << i;
        }
        else {
            ss << " " << i;
        }
        for (uint32_t j = 0; j < device_count; j++) {
            ss << " " << matrix[i][j] << "  ";
        }
        ss << "\n";
    }
    return ss.str();
}
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

std::string topo_manager::to_string() {
    std::stringstream ss;
    ss << " { ";
    ss << "intra_card_colors: ";
    std::copy(
        intra_card_colors.begin(), intra_card_colors.end(), std::ostream_iterator<int>(ss, ", "));
    ss << "\n";
    ss << "inter_card_colors: ";
    std::copy(
        inter_card_colors.begin(), inter_card_colors.end(), std::ostream_iterator<int>(ss, ", "));
    ss << "\n";
    ss << " }";

    return ss.str();
}

} // namespace ccl
