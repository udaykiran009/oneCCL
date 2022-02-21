#include "common/global/global.hpp"
#include "exec/exec.hpp"
#include "topology/topo_manager.hpp"

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include "common/utils/sycl_utils.hpp"
#include "sched/entry/ze/ze_primitives.hpp"
#include <CL/sycl/backend/level_zero.hpp>
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

namespace ccl {

topo_rank_info::topo_rank_info() : rank(ccl_comm::invalid_rank) {
    memset(uuid, 0, topo_uuid_len);
}

topo_host_info::topo_host_info(int idx, const std::string& name, const std::set<int>& ranks)
        : idx(idx),
          name(name),
          ranks(ranks) {}

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
std::string to_string(const p2p_matrix_t& matrix) {
    CCL_THROW_IF_NOT(!matrix.empty());

    uint32_t row_count = matrix.size();
    uint32_t column_count = matrix[0].size();

    constexpr int first_double_digit_number = 10;

    CCL_THROW_IF_NOT(row_count == column_count,
                     "p2p matrix should be square but got [",
                     row_count,
                     "x",
                     column_count,
                     "]");

    std::stringstream ss;
    for (uint32_t j = 0; j < column_count; j++) {
        if (j >= first_double_digit_number) {
            ss << "  " << j;
        }
        else {
            ss << "   " << j;
        }
    }
    ss << "\n";

    for (uint32_t i = 0; i < row_count; i++) {
        if (i >= first_double_digit_number) {
            ss << i;
        }
        else {
            ss << " " << i;
        }
        for (uint32_t j = 0; j < column_count; j++) {
            ss << " " << matrix[i][j] << "  ";
        }
        ss << "\n";
    }

    return ss.str();
}
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

std::string to_string(const domains_t& domains) {
    std::stringstream ss;

    ss << "\n{\n";

    for (const auto& domain : domains) {
        auto& subdomains = domain.second;
        for (size_t subdomain_idx = 0; subdomain_idx < subdomains.size(); subdomain_idx++) {
            auto& subdomain = subdomains[subdomain_idx];
            for (size_t proc_idx = 0; proc_idx < subdomain.size(); proc_idx++) {
                if (subdomain_idx == 0 && proc_idx == 0) {
                    if (domain.first == topo_manager::card_domain_idx)
                        ss << "  card:  ";
                    else if (domain.first == topo_manager::plane_domain_idx)
                        ss << "  plane: ";
                }

                if (proc_idx == 0) {
                    ss << "{ ";
                }

                ss << subdomain[proc_idx] << " ";

                if (proc_idx == subdomain.size() - 1) {
                    ss << "} ";
                }
            }
        }
        ss << "\n";
    }

    ss << "}\n";

    return ss.str();
}

constexpr int topo_manager::invalid_color;
constexpr int topo_manager::max_domain_count;

void topo_manager::init(std::shared_ptr<atl_base_comm> atl_comm,
                        std::shared_ptr<ccl::device> device,
                        std::shared_ptr<ccl::context> context) {
    base_init(atl_comm, device, context);
    if (device) {
        // TODO: move intra/inter card logic under ZE define
        post_init();
    }
}

int topo_manager::get_host_idx() const {
    return host_idx;
}

int topo_manager::get_intra_card_color(int rank) const {
    return intra_card_colors[rank];
}

int topo_manager::get_inter_card_color(int rank) const {
    return inter_card_colors[rank];
}

std::string topo_manager::get_uuid(int rank) const {
    return uuids[rank];
}

bool topo_manager::has_same_ppn() const {
    return is_same_ppn;
}

bool topo_manager::has_same_domains() const {
    return is_same_domains;
}

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
bool topo_manager::has_failed_ports() const {
    return (port_status == port_health_status::fail);
}

bool topo_manager::has_p2p_access() const {
    return is_p2p_access_enabled;
}

std::vector<ze_device_uuid_t> topo_manager::copy_dev_uuids(const rank_info_vec_t& info_vec) const {
    CCL_THROW_IF_NOT(!ze_rank_info_vec.empty());
    CCL_THROW_IF_NOT(!info_vec.empty());
    CCL_THROW_IF_NOT(info_vec.size() <= ze_rank_info_vec.size());
    std::vector<ze_device_uuid_t> result;
    for (auto const& info : info_vec) {
        result.push_back(ze_rank_info_vec[info.rank].device_uuid);
    }
    return result;
}

std::vector<ze_device_handle_t> topo_manager::get_filtered_devices(
    const std::vector<ze::device_info>& node_devices) const {
    CCL_THROW_IF_NOT(!node_devices.empty());

    const auto& host_rank_info_vec = get_filtered_rank_info_vec(host_idx);

    // get comm_device_uuids - uuids from ranks on this host
    const auto& comm_dev_uuids = copy_dev_uuids(host_rank_info_vec);

    // check if comm_device_uuids are in global dev uuids
    CCL_THROW_IF_NOT(is_sub_vector(copy_dev_uuids(rank_info_vec), comm_dev_uuids),
                     "comm_dev_uuids should be sub vector of global device uuids");

    // check if comm_device_uuids are in node_dev_uuids
    std::vector<ze_device_uuid_t> node_dev_uuids;
    for (const auto& node_device : node_devices) {
        node_dev_uuids.push_back(node_device.uuid);
    }
    CCL_THROW_IF_NOT(is_sub_vector(node_dev_uuids, comm_dev_uuids),
                     "comm_dev_uuids should be sub vector of node_dev_uuids");

    // get device handles for comm uuids
    std::vector<ze_device_handle_t> result;
    for (const auto& comm_dev_uuid : comm_dev_uuids) {
        for (const auto& node_device : node_devices) {
            if (ze::is_same_dev_uuid(node_device.uuid, comm_dev_uuid)) {
                result.push_back(node_device.device);
                break;
            }
        }
    }

    CCL_THROW_IF_NOT(result.size() == host_rank_info_vec.size(),
                     "unexpected number of filtered devices: ",
                     result.size(),
                     ", expected: ",
                     host_rank_info_vec.size());
    return result;
}

p2p_matrix_t topo_manager::build_p2p_matrix(const std::vector<ze_device_handle_t>& devices) {
    size_t device_count = devices.size();
    p2p_matrix_t matrix(device_count);

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

bool topo_manager::is_sub_vector(const std::vector<ze_device_uuid_t>& vec,
                                 const std::vector<ze_device_uuid_t>& sub_vec) {
    CCL_THROW_IF_NOT(!vec.empty());
    CCL_THROW_IF_NOT(!sub_vec.empty());

    std::vector<ze_device_uuid_t> unique_sub_vec;
    for (const auto& uuid : sub_vec) {
        bool is_unique_uuid =
            (std::find_if(
                 unique_sub_vec.begin(), unique_sub_vec.end(), [&uuid](const ze_device_uuid_t& u) {
                     return ze::is_same_dev_uuid(uuid, u);
                 }) == unique_sub_vec.end());

        if (is_unique_uuid) {
            unique_sub_vec.push_back(uuid);
        }
    }

    if (unique_sub_vec.size() > vec.size()) {
        LOG_DEBUG("unique sub vector size can not be greater than base vector size: unique: ",
                  unique_sub_vec.size(),
                  ", vec: ",
                  vec.size());
        return false;
    }

    for (const auto& uuid : sub_vec) {
        if (std::find_if(vec.begin(), vec.end(), [&uuid](const ze_device_uuid_t& u) {
                return ze::is_same_dev_uuid(uuid, u);
            }) == vec.end()) {
            return false;
        }
    }
    return true;
}
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

std::string topo_manager::generate_uuid() {
    std::stringstream ss;
    int pid = getpid();

    auto time = std::chrono::high_resolution_clock::now();
    uint64_t time_usec = std::chrono::duration<double, std::micro>(time.time_since_epoch()).count();

    srand(time_usec);
    int random_number = rand();

    constexpr int part_1_len = 8;
    constexpr int part_2_len = 16;

    ss << std::right << std::setfill('0') << std::setw(part_1_len)
       << std::to_string(pid).substr(0, part_1_len) << "-" << std::setw(part_1_len)
       << std::to_string(random_number).substr(0, part_1_len) << "-" << std::setw(part_2_len)
       << std::to_string(time_usec).substr(0, part_2_len);

    std::string result = ss.str();

    size_t expected_uuid_len = (topo_uuid_len - 1);
    CCL_THROW_IF_NOT(result.size() == expected_uuid_len,
                     "unexpected uuid len ",
                     result.size(),
                     ", expected ",
                     expected_uuid_len,
                     ", uuid ",
                     result);
    return result;
}

domains_t topo_manager::parse_topo_env() {
    char* env_to_parse = getenv(CCL_TOPO_COLOR);
    CCL_THROW_IF_NOT(env_to_parse, CCL_TOPO_COLOR, " var is unavailable");
    domains_t domains;

    std::vector<std::string> domain_raw_strs;
    ccl::utils::str_to_array(std::string(env_to_parse), ";", domain_raw_strs);
    check_domain_count(domain_raw_strs.size());

    std::vector<std::map<int, std::string>> domain_strs;
    domain_strs.push_back(get_domain_string(domain_raw_strs[topo_manager::card_domain_idx],
                                            std::string(topo_manager::card_domain_name)));
    domain_strs.push_back(get_domain_string(domain_raw_strs[topo_manager::plane_domain_idx],
                                            std::string(topo_manager::plane_domain_name)));

    for (const auto& domain_str : domain_strs) {
        for (const auto& domain_pair : domain_str) {
            std::vector<std::vector<int>> proc_indexes;
            auto substrs = get_subdomain_strings(domain_pair.second);
            for (const auto& substr : substrs) {
                std::vector<int> procs{};
                ccl::utils::str_to_array(substr, ",", procs);
                for (const auto& proc : procs) {
                    const auto local_proc_count =
                        ccl::global_data::get().executor->get_local_proc_count();
                    CCL_THROW_IF_NOT(proc < local_proc_count,
                                     "unexpected process number: ",
                                     proc,
                                     ", it should be less than: ",
                                     local_proc_count);
                }
                proc_indexes.push_back(procs);
            }
            domains.insert({ domain_pair.first, proc_indexes });
        }
    }
    check_domain_count(domains.size());
    return domains;
}

std::string topo_manager::to_string() {
    std::stringstream ss;

    ss << "\n{\n"
       << "  comm_size: " << comm->get_size() << "\n"
       << "  single_node: " << is_single_node << "\n"
       << "  single_card: " << is_single_card << "\n"
       << "  host_rank_counts: ";

    std::vector<size_t> host_rank_counts;
    for (const auto& info : host_info_vec) {
        host_rank_counts.push_back(info.ranks.size());
    }
    std::copy(
        host_rank_counts.begin(), host_rank_counts.end(), std::ostream_iterator<int>(ss, " "));
    ss << "\n";

    ss << "  intra_card_colors: ";
    std::copy(
        intra_card_colors.begin(), intra_card_colors.end(), std::ostream_iterator<int>(ss, " "));
    ss << "\n";

    ss << "  inter_card_colors: ";
    std::copy(
        inter_card_colors.begin(), inter_card_colors.end(), std::ostream_iterator<int>(ss, " "));
    ss << "\n";

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    ss << "  p2p_access: " << is_p2p_access_enabled << "\n";
    if (port_status != port_health_status::unknown) {
        ss << "  ports_healthy: " << ((port_status == port_health_status::ok) ? "1" : "0") << "\n";
    }
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    ss << "}";

    return ss.str();
}

bool topo_manager::check_colors() const {
    bool expected_colors = true;
    for (size_t domain_idx = 0; domain_idx < topo_manager::max_domain_count; domain_idx++) {
        CCL_THROW_IF_NOT(domain_idx == topo_manager::card_domain_idx ||
                             domain_idx == topo_manager::plane_domain_idx,
                         "unknown domain_idx detected");
        int max_ranks_per_subdomain = (domain_idx == topo_manager::card_domain_idx)
                                          ? topo_manager::max_ranks_per_card
                                          : topo_manager::max_ranks_per_plane;
        const std::vector<int>& colors =
            (domain_idx == topo_manager::card_domain_idx) ? intra_card_colors : inter_card_colors;
        std::string color_name = (domain_idx == topo_manager::card_domain_idx)
                                     ? topo_manager::card_domain_name
                                     : topo_manager::plane_domain_name;

        CCL_THROW_IF_NOT(
            std::find(colors.begin(), colors.end(), topo_manager::invalid_color) == colors.end(),
            color_name,
            " domain colors data is not valid");

        std::set<int> unique_colors(colors.begin(), colors.end());

        std::vector<int> counts;
        for (auto unique_color : unique_colors) {
            counts.push_back(std::count(colors.begin(), colors.end(), unique_color));
        }

        expected_colors &=
            std::all_of(counts.begin(), counts.end(), [&counts, &max_ranks_per_subdomain](int c) {
                return (counts.front() == c && c <= max_ranks_per_subdomain);
            });
    }
    return expected_colors;
}

void topo_manager::allgather(const void* send_buf, void* recv_buf, int bytes) {
    std::vector<int> recv_bytes(comm->get_size(), bytes);
    allgatherv(send_buf, recv_buf, recv_bytes);
}

void topo_manager::allgatherv(const void* send_buf,
                              void* recv_buf,
                              const std::vector<int>& recv_bytes) {
    atl_req_t req{};

    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    CCL_THROW_IF_NOT((int)recv_bytes.size() == comm->get_size(),
                     "unexpected recv_bytes size ",
                     recv_bytes.size(),
                     ", comm_size ",
                     comm_size);

    std::vector<int> offsets(comm_size, 0);
    for (int i = 1; i < comm_size; i++) {
        offsets[i] = offsets[i - 1] + recv_bytes[i - 1];
    }

    comm->allgatherv(0 /* ep_idx */,
                     send_buf,
                     recv_bytes[comm_rank],
                     recv_buf,
                     recv_bytes.data(),
                     offsets.data(),
                     req);
    comm->wait(0 /* ep_idx */, req);
}

void topo_manager::fill_env_colors(const rank_info_vec_t& info_vec) {
    for (const auto& domain : domains) {
        auto& subdomains = domain.second;
        int color_idx = 0;
        for (auto& subdomain : subdomains) {
            rank_info_vec_t filtered_info_vec;
            for (size_t proc_idx = 0; proc_idx < subdomain.size(); proc_idx++) {
                for (size_t i = 0; i < info_vec.size(); i++) {
                    if (info_vec[i].local_proc_idx == subdomain[proc_idx]) {
                        filtered_info_vec.push_back(info_vec[i]);
                    }
                }
            }

            for (const auto& info : filtered_info_vec) {
                if (domain.first == topo_manager::card_domain_idx) {
                    check_invalid_color(intra_card_colors[info.rank]);
                    intra_card_colors[info.rank] = color_idx;
                }
                else if (domain.first == topo_manager::plane_domain_idx) {
                    check_invalid_color(inter_card_colors[info.rank]);
                    inter_card_colors[info.rank] = color_idx;
                }
            }
            color_idx++;
        }
    }
}

void topo_manager::fill_fixed_colors(const rank_info_vec_t& info_vec) {
    for (size_t idx = 0; idx < info_vec.size(); idx++) {
        auto rank = info_vec[idx].rank;
        check_invalid_color(intra_card_colors[rank]);
        check_invalid_color(inter_card_colors[rank]);
        intra_card_colors[rank] = idx / 2;
        inter_card_colors[rank] = idx % 2;
    }
}

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
void topo_manager::fill_ze_colors() {
    CCL_THROW_IF_NOT(!host_info_vec.empty());
    for (int h_idx = 0; h_idx < (int)host_info_vec.size(); h_idx++) {
        fill_ze_intra_colors(get_filtered_rank_info_vec(h_idx));
    }
    fill_ze_inter_colors();
}

void topo_manager::fill_ze_intra_colors(const rank_info_vec_t& local_info_vec) {
    CCL_THROW_IF_NOT(!local_info_vec.empty());
    CCL_THROW_IF_NOT(!ze_rank_info_vec.empty());
    int card_idx = 0;
    for (size_t idx = 0; idx < local_info_vec.size(); idx++) {
        size_t card_size = 0;
        const topo_rank_info& info = local_info_vec[idx];
        int rank = info.rank;

        // check if intra's elem is already filled
        if (intra_card_colors[rank] != topo_manager::invalid_color) {
            continue;
        }

        // iterate over local info comparing pci addresses for current and peer ranks
        for (size_t peer_idx = idx; peer_idx < local_info_vec.size(); peer_idx++) {
            const topo_rank_info& peer_info = local_info_vec[peer_idx];
            int peer_rank = peer_info.rank;
            // if rank & peer rank is on the card
            if (ze::is_same_pci_addr(ze_rank_info_vec[rank].pci_addr,
                                     ze_rank_info_vec[peer_rank].pci_addr)) {
                // fill intra colors
                check_invalid_color(intra_card_colors[peer_rank]);

                intra_card_colors[peer_rank] = card_idx;
                card_size++;
                // if size reached card_limit
                if (card_size == max_ranks_per_card) {
                    // go to the next card
                    card_idx++;
                    // reset card size
                    card_size = 0;
                    break;
                }
            }
        }
    }
}

void topo_manager::fill_ze_inter_colors() {
    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    CCL_THROW_IF_NOT(fabric_ports.empty() || ((int)fabric_ports.size() == comm_size));
    CCL_THROW_IF_NOT(!host_info_vec.empty());

    for (const auto& host_info : host_info_vec) {
        std::vector<std::set<int>> planes;

        for (auto rank : host_info.ranks) {
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

            if (fabric_ports.empty()) {
                continue;
            }

            for (const auto& rank_port : fabric_ports[rank]) {
                for (auto peer_rank : host_info.ranks) {
                    if (rank == peer_rank) {
                        // skip my ports
                        continue;
                    }
                    for (const auto& peer_port : fabric_ports[peer_rank]) {
                        if (ze::is_same_fabric_port(peer_port.local, rank_port.remote)) {
                            planes[cur_plane_idx].insert(peer_rank);
                            break;
                        }
                    }
                }
            }
        }

        if (comm_rank == 0) {
            for (int plane_idx = 0; plane_idx < (int)planes.size(); plane_idx++) {
                std::stringstream ss;
                std::copy(planes[plane_idx].begin(),
                          planes[plane_idx].end(),
                          std::ostream_iterator<int>(ss, " "));
                LOG_INFO(
                    "host: ", host_info.idx, ", plane ", plane_idx, " contains ranks: ", ss.str());
            }
        }

        bool single_rank_planes = std::all_of(planes.begin(), planes.end(), [](std::set<int>& p) {
            return (p.size() == 1);
        });

        size_t port_count = 0;
        if (!fabric_ports.empty()) {
            for (auto rank : host_info.ranks) {
                port_count += fabric_ports[rank].size();
            }
        }

        if (single_rank_planes && (port_count == 0)) {
            // use simple separation of ranks between planes
            // ranks with the same intra_card color
            // should be placed to different planes
            fill_ze_inter_colors(get_filtered_rank_info_vec(host_info.idx));
        }
        else {
            // take into account fabric planes information
            fill_ze_inter_colors(planes);
        }
    }
}

void topo_manager::fill_ze_inter_colors(const rank_info_vec_t& local_info_vec) {
    for (const auto& info : local_info_vec) {
        int rank = info.rank;

        if (inter_card_colors[rank] != topo_manager::invalid_color) {
            continue;
        }

        int plane_idx = 0;
        for (const auto& peer_info : local_info_vec) {
            int peer_rank = peer_info.rank;
            if (intra_card_colors[rank] == intra_card_colors[peer_rank]) {
                check_invalid_color(inter_card_colors[peer_rank]);
                inter_card_colors[peer_rank] = plane_idx;
                plane_idx++;
            }
        }
    }
}

void topo_manager::fill_ze_inter_colors(const std::vector<std::set<int>>& planes) {
    for (int rank = 0; rank < comm->get_size(); rank++) {
        for (int plane_idx = 0; plane_idx < (int)planes.size(); plane_idx++) {
            if (planes[plane_idx].find(rank) != planes[plane_idx].end()) {
                check_invalid_color(inter_card_colors[rank]);
                inter_card_colors[rank] = plane_idx;
                break;
            }
        }
    }
}

bool topo_manager::check_p2p_access() const {
    for (size_t i = 0; i < p2p_matrix.size(); i++) {
        for (size_t j = 0; j < p2p_matrix[i].size(); j++) {
            if (!p2p_matrix[i][j]) {
                return false;
            }
        }
    }
    return true;
}

fabric_ports_t topo_manager::get_fabric_ports() {
    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    CCL_THROW_IF_NOT(fabric_ports.empty());
    CCL_THROW_IF_NOT(host_idx != topo_manager::invalid_host_idx);
    CCL_THROW_IF_NOT(!host_info_vec.empty());
    CCL_THROW_IF_NOT(ze_device);

    uint32_t port_count{};

    ZE_CALL(zesDeviceEnumFabricPorts, ((zes_device_handle_t)ze_device, &port_count, NULL));
    std::vector<zes_fabric_port_handle_t> ports(port_count);
    ZE_CALL(zesDeviceEnumFabricPorts, ((zes_device_handle_t)ze_device, &port_count, ports.data()));

    // TODO: to be deleted, will rely on real port count
    char* all_ports_env = getenv("CCL_TOPO_ALL_PORTS");

    std::vector<topo_ze_port_info> my_ports;
    for (const auto& port : ports) {
        zes_fabric_port_properties_t prop;
        ZE_CALL(zesFabricPortGetProperties, (port, &prop));

        zes_fabric_port_config_t config;
        ZE_CALL(zesFabricPortGetConfig, (port, &config));

        if (!config.enabled) {
            LOG_INFO("(",
                     host_info_vec[host_idx].name,
                     ") rank ",
                     comm_rank,
                     " has disabled port ",
                     ccl::ze::to_string(prop.portId),
                     " skip it");
            continue;
        }

        zes_fabric_port_state_t state;
        ZE_CALL(zesFabricPortGetState, (port, &state));

        if (state.status == ZES_FABRIC_PORT_STATUS_HEALTHY ||
            state.status == ZES_FABRIC_PORT_STATUS_DEGRADED ||
            state.status == ZES_FABRIC_PORT_STATUS_FAILED) {
            // port is connected
            if ((all_ports_env && atoi(all_ports_env)) ||
                (prop.onSubdevice && prop.subdeviceId == dev_props.subdeviceId)) {
                my_ports.push_back({ host_idx, prop.portId, state.remotePortId, state.status });
            }

            if (state.status == ZES_FABRIC_PORT_STATUS_DEGRADED ||
                state.status == ZES_FABRIC_PORT_STATUS_FAILED) {
                LOG_WARN("host: ",
                         host_info_vec[host_idx].name,
                         ", rank: ",
                         comm_rank,
                         ", port issue: ",
                         ccl::ze::to_string(state));
            }
        }
    }

    // exchange port counts
    int my_port_count = (int)my_ports.size();
    std::vector<int> all_port_counts(comm_size);

    allgather(&my_port_count, all_port_counts.data(), sizeof(my_port_count));

    size_t total_port_count = std::accumulate(all_port_counts.begin(), all_port_counts.end(), 0);

    if (total_port_count == 0) {
        LOG_DEBUG("no ports detected");
        return {};
    }
    else {
        // assume all ports are functional
        // until at least single failed port
        // within comm will be found
        port_status = port_health_status::ok;
    }

    // exchange port info
    std::vector<int> recv_bytes(comm_size, sizeof(topo_ze_port_info) * all_port_counts[0]);
    std::vector<int> port_count_offsets(comm_size, 0);

    for (int i = 1; i < comm_size; i++) {
        recv_bytes[i] = sizeof(topo_ze_port_info) * all_port_counts[i];
        port_count_offsets[i] = port_count_offsets[i - 1] + all_port_counts[i - 1];
    }

    std::vector<topo_ze_port_info> all_ports(total_port_count);

    allgatherv(my_ports.data(), all_ports.data(), recv_bytes);

    // print all ports before filtering
    if (comm_rank == 0) {
        for (int rank = 0; rank < comm_size; rank++) {
            if (all_port_counts[rank]) {
                LOG_INFO("--- rank ", rank, ", host_idx ", host_idx, " ---");
            }

            for (int port_idx = port_count_offsets[rank];
                 port_idx < port_count_offsets[rank] + all_port_counts[rank];
                 port_idx++) {
                LOG_INFO("port: status: ",
                         ccl::ze::to_string(all_ports[port_idx].local_status),
                         ", local_id: ",
                         ccl::ze::to_string(all_ports[port_idx].local),
                         ", remote_id: ",
                         ccl::ze::to_string(all_ports[port_idx].remote));
            }
        }
    }

    // keep only comm-local ports (ports which have pairs within comm)
    fabric_ports_t comm_ports(comm_size);
    for (int h_idx = 0; h_idx < (int)host_info_vec.size(); h_idx++) {
        std::set<zes_fabric_port_id_t, ccl::ze::fabric_port_comparator> host_port_ids;
        for (const auto& port : all_ports) {
            if (port.host_idx == h_idx) {
                host_port_ids.insert(port.local);
            }
        }
        CCL_THROW_IF_NOT(!host_port_ids.empty());

        for (auto rank : host_info_vec[h_idx].ranks) {
            for (int port_idx = port_count_offsets[rank];
                 port_idx < port_count_offsets[rank] + all_port_counts[rank];
                 port_idx++) {
                const auto& port = all_ports[port_idx];
                if (host_port_ids.find(port.remote) != host_port_ids.end()) {
                    comm_ports[rank].push_back(port);
                }
                else {
                    LOG_DEBUG("host_idx ",
                              h_idx,
                              ": local port ",
                              ccl::ze::to_string(port.local),
                              " is connected with unknown remote port ",
                              ccl::ze::to_string(port.remote));
                }
            }
        }
    }

    size_t total_comm_ports_count = 0;
    for (const auto& rank_ports : comm_ports) {
        total_comm_ports_count += rank_ports.size();
    }
    CCL_THROW_IF_NOT(total_comm_ports_count <= all_ports.size(),
                     "unexpected comm_ports count: ",
                     total_comm_ports_count,
                     ", expected not greater than: ",
                     all_ports.size());

    // report about failed ports and remove them
    size_t failed_port_count = 0;
    for (int rank = 0; rank < comm_size; rank++) {
        auto& rank_ports = comm_ports[rank];
        for (const auto& port : rank_ports) {
            if (port.local_status == ZES_FABRIC_PORT_STATUS_FAILED) {
                LOG_DEBUG("rank: ",
                          rank,
                          ", port ",
                          ccl::ze::to_string(port.local),
                          " is in failed status, skipping");
                failed_port_count++;
            }
        }

        rank_ports.erase(std::remove_if(rank_ports.begin(),
                                        rank_ports.end(),
                                        [](topo_ze_port_info p) {
                                            return p.local_status == ZES_FABRIC_PORT_STATUS_FAILED;
                                        }),
                         rank_ports.end());
    }

    if (failed_port_count) {
        port_status = port_health_status::fail;
        LOG_INFO("removed ", failed_port_count, " failed ports from topology");
    }

    // make sure that no more failed ports in final container
    for (const auto& rank_ports : comm_ports) {
        for (const auto& port : rank_ports) {
            CCL_THROW_IF_NOT(port.local_status != ZES_FABRIC_PORT_STATUS_FAILED,
                             "unexpected port status");
        }
    }

    return comm_ports;
}
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

rank_info_vec_t topo_manager::get_filtered_rank_info_vec(int filter_host_idx) const {
    CCL_THROW_IF_NOT(!rank_info_vec.empty());
    rank_info_vec_t info_vec;
    std::copy_if(rank_info_vec.begin(),
                 rank_info_vec.end(),
                 std::back_inserter(info_vec),
                 [filter_host_idx](const topo_rank_info& info) {
                     return (info.host_idx == filter_host_idx);
                 });
    return info_vec;
}

void topo_manager::check_invalid_color(int color) {
    CCL_THROW_IF_NOT(color == topo_manager::invalid_color,
                     "unexpected color value: ",
                     color,
                     ", expected: ",
                     topo_manager::invalid_color);
}

void topo_manager::check_domain_count(size_t domain_count) {
    CCL_THROW_IF_NOT(domain_count == topo_manager::max_domain_count,
                     "unexpected domain count:",
                     domain_count,
                     ", expected:",
                     topo_manager::max_domain_count);
}

std::map<int, std::string> topo_manager::get_domain_string(const std::string& input_str,
                                                           const std::string& key) {
    std::map<int, std::string> map;
    auto str = input_str;

    size_t pos = str.find(key);
    if (pos != std::string::npos) {
        str.erase(pos, key.length() + 1); // + :
    }

    int domain_idx = topo_manager::invalid_domain_idx;
    if (key == std::string(topo_manager::card_domain_name)) {
        domain_idx = topo_manager::card_domain_idx;
    }
    else if (key == std::string(topo_manager::plane_domain_name)) {
        domain_idx = topo_manager::plane_domain_idx;
    }

    CCL_THROW_IF_NOT(
        domain_idx != topo_manager::invalid_domain_idx, "unexpected domain index: ", domain_idx);
    map.insert({ domain_idx, str });
    return map;
}

std::vector<std::string> topo_manager::get_subdomain_strings(const std::string& input_str) {
    std::vector<std::string> results;
    auto str = input_str;
    bool can_parse = true;
    do {
        auto substring = ccl::utils::get_substring_between_delims(str, "{", "}");
        results.push_back(substring);

        size_t pos = str.find(substring);
        if (pos != std::string::npos) {
            str.erase(0, str.find("}") + 1);
        }

        if (str.empty())
            can_parse = false;
    } while (can_parse);
    return results;
}

void topo_manager::build_host_info() {
    CCL_THROW_IF_NOT(host_info_vec.empty());

    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    std::vector<char> all_hostnames_raw(max_hostname_len * comm_size);
    char my_hostname[max_hostname_len] = { 0 };

    gethostname(my_hostname, max_hostname_len - 1);
    LOG_DEBUG("rank: ", comm_rank, ", size: ", comm_size, ", host: ", my_hostname);

    allgather(my_hostname, all_hostnames_raw.data(), max_hostname_len);

    std::vector<std::string> all_hostnames(comm_size);
    std::set<std::string> unique_hostnames;
    for (int idx = 0; idx < comm_size; idx++) {
        auto str =
            std::string(all_hostnames_raw.data() + (idx * max_hostname_len), max_hostname_len);
        str.erase(std::find(str.begin(), str.end(), '\0'), str.end());
        unique_hostnames.insert(str);
        all_hostnames[idx] = str;
    }
    CCL_THROW_IF_NOT(!unique_hostnames.empty(), "empty unique_hostnames");

    CCL_THROW_IF_NOT(unique_hostnames.find(my_hostname) != unique_hostnames.end(),
                     "unique_hostnames does not include my_hostname ",
                     my_hostname);
    host_idx = std::distance(unique_hostnames.begin(), unique_hostnames.find(my_hostname));
    CCL_THROW_IF_NOT((host_idx != topo_manager::invalid_host_idx) && (host_idx >= 0),
                     "invalid host index, host: ",
                     my_hostname);

    for (const auto& hostname : unique_hostnames) {
        size_t unique_host_idx =
            std::distance(unique_hostnames.begin(), unique_hostnames.find(hostname));
        host_info_vec.push_back({ (int)unique_host_idx, hostname });
        CCL_THROW_IF_NOT(unique_host_idx == (host_info_vec.size() - 1));
    }

    for (int rank = 0; rank < comm_size; rank++) {
        const auto& rank_hostname = all_hostnames[rank];
        int rank_host_idx =
            std::distance(unique_hostnames.begin(), unique_hostnames.find(rank_hostname));
        host_info_vec[rank_host_idx].ranks.insert(rank);
    }

    for (int h_idx = 0; h_idx < (int)host_info_vec.size(); h_idx++) {
        const auto& info = host_info_vec[h_idx];
        CCL_THROW_IF_NOT(!info.ranks.empty() && (int)info.ranks.size() <= comm_size,
                         "host_idx: ",
                         info.idx,
                         ", unexpected number of ranks: ",
                         info.ranks.size());
        CCL_THROW_IF_NOT(
            info.idx == h_idx, "unexpected host_idx: ", info.idx, ", expected: ", h_idx);
    }

    is_single_node = (host_info_vec.size() == 1) ? true : false;
    is_single_card = is_single_node && (comm_size <= max_ranks_per_card);
    is_same_ppn =
        std::all_of(host_info_vec.begin(), host_info_vec.end(), [this](const topo_host_info& info) {
            return (info.ranks.size() == host_info_vec.front().ranks.size());
        });
}

void topo_manager::base_init(std::shared_ptr<atl_base_comm> atl_comm,
                             std::shared_ptr<ccl::device> device,
                             std::shared_ptr<ccl::context> context) {
    comm = atl_comm;

    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    intra_card_colors.resize(comm_size, topo_manager::invalid_color);
    inter_card_colors.resize(comm_size, topo_manager::invalid_color);
    uuids.resize(comm_size);
    rank_info_vec.resize(comm_size);

    build_host_info();

    // exchange common rank info
    topo_rank_info rank_info{};
    rank_info.rank = comm_rank;
    rank_info.host_idx = host_idx;
    rank_info.local_proc_idx = ccl::global_data::get().executor->get_local_proc_idx();
    std::string rank_uuid = topo_manager::generate_uuid();
    std::copy(rank_uuid.begin(), rank_uuid.end(), rank_info.uuid);

    allgather(&rank_info, rank_info_vec.data(), sizeof(rank_info));

    for (size_t idx = 0; idx < rank_info_vec.size(); idx++) {
        uuids[idx] = std::string(rank_info_vec[idx].uuid);
        CCL_THROW_IF_NOT(rank_info_vec[idx].rank == (int)idx,
                         "unexpected rank_info_vec rank: ",
                         rank_info_vec[idx].rank,
                         ", expected: ",
                         idx);
    }

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    ze_base_init(device, context);
    is_p2p_access_enabled = check_p2p_access();
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    if (!(device && context)) {
        return;
    }

    if (ccl::global_data::env().topo_color == topo_color_mode::fixed) {
        for (int h_idx = 0; h_idx < (int)host_info_vec.size(); h_idx++) {
            fill_fixed_colors(get_filtered_rank_info_vec(h_idx));
        }
    }
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    else if (ccl::global_data::env().topo_color == topo_color_mode::ze) {
        fill_ze_colors();
    }
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    else if (ccl::global_data::env().topo_color == topo_color_mode::env) {
        domains = parse_topo_env();

        CCL_THROW_IF_NOT(!domains.empty(), "domains container is empty");
        LOG_INFO("domains: ", ccl::to_string(domains));

        for (int h_idx = 0; h_idx < (int)host_info_vec.size(); h_idx++) {
            fill_env_colors(get_filtered_rank_info_vec(h_idx));
        }
    }
    else {
        CCL_THROW("unknown topo color mode: ", (int)ccl::global_data::env().topo_color);
    }

    // update is_single_card
    bool no_invalid_colors = (std::find(intra_card_colors.begin(),
                                        intra_card_colors.end(),
                                        topo_manager::invalid_color) == intra_card_colors.end())
                                 ? true
                                 : false;
    bool all_same_colors =
        std::all_of(intra_card_colors.begin(), intra_card_colors.end(), [this](int c) {
            return (c == intra_card_colors.front());
        });

    is_single_card = (is_single_node && (comm->get_size() <= topo_manager::max_ranks_per_card) &&
                      no_invalid_colors && all_same_colors);

    LOG_INFO("rank: ",
             comm_rank,
             ", size: ",
             comm_size,
             ", uuid: ",
             rank_info.uuid,
             ", host: { name: ",
             host_info_vec[host_idx].name,
             ", idx: ",
             host_idx,
             " }");
}

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
void topo_manager::ze_base_init(std::shared_ptr<ccl::device> device,
                                std::shared_ptr<ccl::context> context) {
    CCL_THROW_IF_NOT(comm);

    int comm_rank = comm->get_rank();
    int comm_size = comm->get_size();

    p2p_matrix.resize(comm_size);
    for (size_t i = 0; i < p2p_matrix.size(); i++) {
        p2p_matrix[i].resize(comm_size, false);
    }

    if (!(device && context)) {
        return;
    }

    if (device.get()->get_native().get_backend() != utils::get_level_zero_backend()) {
        return;
    }

    ze_device = sycl::get_native<utils::get_level_zero_backend()>(device.get()->get_native());
    CCL_THROW_IF_NOT(ze_device, "null ze device");
    ZE_CALL(zeDeviceGetProperties, (ze_device, &dev_props));

    // exchange ze specific rank info
    topo_ze_rank_info ze_rank_info{};
    ze_rank_info_vec.resize(comm_size);

    ze_rank_info.device_uuid = dev_props.uuid;

    zes_pci_properties_t pci_props = {};
    ZE_CALL(zesDevicePciGetProperties, ((zes_device_handle_t)ze_device, &pci_props));
    ze_rank_info.pci_addr = pci_props.address;

    allgather(&ze_rank_info, ze_rank_info_vec.data(), sizeof(ze_rank_info));

    // build fabric port info
    fabric_ports = get_fabric_ports();

    // build p2p connectivity info
    const auto& node_devices = global_data::get().ze_data->devices;
    p2p_matrix = build_p2p_matrix(get_filtered_devices(node_devices));
    LOG_DEBUG("p2p matrix: \n",
              ccl::to_string(p2p_matrix),
              "\nnumber of node devices: ",
              node_devices.size());

    uint32_t subdev_count{};
    ZE_CALL(zeDeviceGetSubDevices, (ze_device, &subdev_count, nullptr));

    LOG_INFO("rank: ",
             comm_rank,
             ", size: ",
             comm_size,
             ", device uuid: ",
             ccl::ze::to_string(ze_rank_info.device_uuid),
             ", subdevices: ",
             subdev_count,
             ", subdevice_id: ",
             ((dev_props.flags & ZE_DEVICE_PROPERTY_FLAG_SUBDEVICE)
                  ? std::to_string(dev_props.subdeviceId)
                  : "na"),
             ", host: { name: ",
             host_info_vec[host_idx].name,
             ", idx: ",
             host_idx,
             " }");
}
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

void topo_manager::post_init() {
    for (int rank = 0; rank < comm->get_size(); rank++) {
        intra_card_colors[rank] += rank_info_vec[rank].host_idx * max_ranks_per_host;
        inter_card_colors[rank] += rank_info_vec[rank].host_idx * max_ranks_per_host;
    }
    is_same_domains = check_colors();
}

} // namespace ccl
