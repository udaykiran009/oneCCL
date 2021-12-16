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
    int rank = atl_comm->get_rank();
    int size = atl_comm->get_size();

    intra_colors.resize(size, topo_manager::invalid_color);
    inter_colors.resize(size, topo_manager::invalid_color);
    p2p_matrix.resize(size);
    for (size_t i = 0; i < p2p_matrix.size(); i++) {
        p2p_matrix[i].resize(size, true);
    }

    std::vector<char> all_hostnames(max_hostname_len * size);
    char my_hostname[max_hostname_len] = { 0 };
    gethostname(my_hostname, max_hostname_len - 1);

    LOG_DEBUG("rank: ", rank, ", size: ", size, "hostname: ", my_hostname);
    int recv_byte = max_hostname_len;
    std::vector<int> recv_bytes(size, recv_byte);
    std::vector<int> offsets(size, 0);
    for (int i = 1; i < size; i++) {
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
    for (int idx = 0; idx < size; idx++) {
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
    std::vector<rank_topo_info> topo_info_vec(size);

    device = sycl::get_native<ccl::utils::get_level_zero_backend()>(device_ptr.get()->get_native());

    rank_info.rank = rank;

    ze_device_properties_t dev_props = ccl::ze::default_device_props;
    ZE_CALL(zeDeviceGetProperties, (device, &dev_props));
    rank_info.uuid = dev_props.uuid;
    LOG_DEBUG(
        "rank: ", rank, ", size: ", size, ", device uuid: ", ccl::ze::to_string(rank_info.uuid));

    zes_pci_properties_t pci_props = {};
    ZE_CALL(zesDevicePciGetProperties, (device, &pci_props));
    rank_info.pci_addr = pci_props.address;

    recv_bytes.clear();
    offsets.clear();
    recv_bytes.resize(size, sizeof(rank_info));
    offsets.resize(size, 0);
    for (int i = 1; i < size; i++) {
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

    int topo_info_size = static_cast<int>(topo_info_vec.size());
    // create groups with its unique color
    for (int i = 0; i < topo_info_size; i++) {
        // skip the marked rank
        if (intra_colors[i] != topo_manager::invalid_color)
            continue;
        intra_colors[i] = i; // create new group with color
        for (int j = i + 1; j <= topo_info_size; j++) {
            // my rank and right neighboor have the same pci address?
            if (is_same_pci_addr(topo_info_vec[i].pci_addr, topo_info_vec[j].pci_addr)) {
                intra_colors[j] = i;
            }
        }
    }

    // paint all ranks in all groups
    int plane_idx = 0;
    // iterate over all groups
    for (int i = 0; i < topo_info_size; i++) {
        plane_idx = 0;
        // iterate over all ranks in one group
        for (int j = 0; j < topo_info_size; j++) {
            if (intra_colors[j] == intra_colors[i]) {
                inter_colors[j] = plane_idx;
                plane_idx++;
            }
        }
    }

    auto devices = global_data::get().ze_data->device_list;
    p2p_matrix = build_p2p_matrix(devices);
    LOG_DEBUG("p2p matrix: \n", to_string(p2p_matrix), "\nnumber of devices: ", devices.size());
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
}

int topo_manager::get_intra_card_color(int rank) {
    int color{};
    if (ccl::global_data::env().topo_color == topo_color_mode::ze) {
        color = intra_colors[rank];
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
        color = inter_colors[rank];
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
    ss << "intra_colors: ";
    std::copy(intra_colors.begin(), intra_colors.end(), std::ostream_iterator<int>(ss, ", "));
    ss << "\n";
    ss << "inter_colors: ";
    std::copy(inter_colors.begin(), inter_colors.end(), std::ostream_iterator<int>(ss, ", "));
    ss << "\n";
    ss << " }";

    return ss.str();
}

} // namespace ccl
