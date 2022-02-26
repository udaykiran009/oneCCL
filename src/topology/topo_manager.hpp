#pragma once

#include "atl/atl_base_comm.hpp"
#include "common/utils/utils.hpp"
#include "oneapi/ccl/config.h"

#include <algorithm>
#include <numeric>
#include <set>
#include <sstream>
#include <string>

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
#include "common/global/ze_data.hpp"
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

namespace ccl {

enum class topo_color_mode : int { fixed, ze, env };
static std::map<topo_color_mode, std::string> topo_color_names = {
    std::make_pair(topo_color_mode::fixed, "fixed"),
    std::make_pair(topo_color_mode::ze, "ze"),
    std::make_pair(topo_color_mode::env, "env")
};

static constexpr int topo_uuid_len = 35;

struct topo_rank_info {
    int rank;
    int host_idx;
    int local_proc_idx;
    char uuid[topo_uuid_len];

    topo_rank_info();
};

struct topo_host_info {
    int idx;
    std::string name;

    // subset of global ranks
    // launched on the same host
    std::set<int> ranks;

    topo_host_info(int idx, const std::string& name, const std::set<int>& ranks = {});
};

using host_info_vec_t = typename std::vector<topo_host_info>;
using rank_info_vec_t = typename std::vector<topo_rank_info>;
std::string to_string(const rank_info_vec_t& rank_info_vec, const host_info_vec_t& host_info_vec);

// each element in map = domain = domain_idx + vector of subdomains
// subdomain = vector of local process indexes
using domains_t = typename std::map<int, std::vector<std::vector<int>>>;
std::string to_string(const domains_t& domains);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
struct topo_ze_rank_info {
    ze_device_uuid_t device_uuid{};
    zes_pci_address_t pci_addr{};
    uint32_t subdev_count{};
    uint32_t subdev_id{};
    ze_device_property_flags_t dev_prop_flags{};
};

struct topo_ze_port_info {
    int host_idx{};

    // fabric_port_id is unique within host only
    zes_fabric_port_id_t local{};
    zes_fabric_port_id_t remote{};

    zes_fabric_port_status_t local_status{};
};

using ze_rank_info_vec_t = typename std::vector<topo_ze_rank_info>;
std::string to_string(const ze_rank_info_vec_t& ze_rank_info_vec,
                      const host_info_vec_t& host_info_vec);

using p2p_matrix_t = typename std::vector<std::vector<bool>>;
using fabric_ports_t = typename std::vector<std::vector<topo_ze_port_info>>;
using plane_t = typename std::set<int>;

std::string to_string(const p2p_matrix_t& p2p_matrix);

#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

class topo_manager {
public:
    static constexpr int invalid_color = -1;
    static constexpr int invalid_host_idx = -1;
    static constexpr int invalid_plane_idx = -1;
    static constexpr int invalid_domain_idx = -1;

    static constexpr int max_hostname_len = 256;
    static constexpr int max_ranks_per_host = 1000;
    static constexpr int max_ranks_per_card = 2;
    static constexpr int max_ranks_per_plane = 8;
    static constexpr int max_domain_count = 2;

    static constexpr int card_domain_idx = 0;
    static constexpr int plane_domain_idx = 1;
    static constexpr const char* card_domain_name = "card";
    static constexpr const char* plane_domain_name = "plane";

    topo_manager() = default;
    topo_manager(const topo_manager& other) = default;
    topo_manager& operator=(const topo_manager& other) = default;

    ~topo_manager() = default;

    void init(std::shared_ptr<atl_base_comm> atl_comm,
              std::shared_ptr<ccl::device> device_ptr,
              std::shared_ptr<ccl::context> context_ptr);

    int get_host_idx() const;
    int get_intra_card_color(int rank) const;
    int get_inter_card_color(int rank) const;
    std::string get_uuid(int rank) const;
    bool has_same_ppn() const;
    bool has_same_domains() const;

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    enum class port_health_status { unknown, ok, fail };
    bool has_failed_ports() const;
    bool has_p2p_access() const;
    std::vector<ze_device_uuid_t> copy_dev_uuids(const rank_info_vec_t& info_vec) const;
    std::vector<ze_device_handle_t> get_filtered_devices(
        const std::vector<ze::device_info>& node_devices) const;
    static p2p_matrix_t build_p2p_matrix(const std::vector<ze_device_handle_t>& devices);
    static bool is_sub_vector(const std::vector<ze_device_uuid_t>& vec,
                              const std::vector<ze_device_uuid_t>& sub_vec);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    static std::string generate_uuid();
    static domains_t parse_topo_env();

    std::string to_string() const;

    bool is_single_node = false;
    bool is_single_card = false;

private:
    bool check_colors() const;

    void allgather(const void* send_buf, void* recv_buf, int bytes);
    void allgatherv(const void* send_buf, void* recv_buf, const std::vector<int>& recv_bytes);

    void fill_env_colors(const rank_info_vec_t& local_info_vec);
    void fill_fixed_colors(const rank_info_vec_t& info_vec);

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    void fill_ze_colors();

    void fill_ze_intra_colors(const rank_info_vec_t& local_info_vec);

    void fill_ze_inter_colors();
    void fill_ze_inter_colors(const rank_info_vec_t& local_info_vec);
    void fill_ze_inter_colors(const std::vector<plane_t>& planes);

    bool check_p2p_access() const;
    fabric_ports_t get_fabric_ports();

    static void check_planes(const std::vector<plane_t>& planes);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE

    rank_info_vec_t get_filtered_rank_info_vec(int filter_host_idx) const;

    static void check_invalid_color(int color);
    static void check_domain_count(size_t domain_count);

    static std::map<int, std::string> get_domain_string(const std::string& input_str,
                                                        const std::string& key);
    static std::vector<std::string> get_subdomain_strings(const std::string& input_str);

    void build_host_info();

    void base_init(std::shared_ptr<atl_base_comm> atl_comm,
                   std::shared_ptr<ccl::device> device,
                   std::shared_ptr<ccl::context> context);
#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    void ze_base_init(std::shared_ptr<ccl::device> device, std::shared_ptr<ccl::context> context);
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
    void post_init();

    std::shared_ptr<atl_base_comm> comm;

    int host_idx = invalid_host_idx;
    host_info_vec_t host_info_vec;

    std::vector<int> intra_card_colors{};
    std::vector<int> inter_card_colors{};
    std::vector<std::string> uuids{};
    rank_info_vec_t rank_info_vec;

    domains_t domains;

    bool is_same_ppn = true;
    bool is_same_domains = true;

#if defined(CCL_ENABLE_SYCL) && defined(CCL_ENABLE_ZE)
    ze_device_handle_t ze_device{};
    ze_device_properties_t dev_props = ccl::ze::default_device_props;
    p2p_matrix_t p2p_matrix;
    fabric_ports_t fabric_ports;
    ze_rank_info_vec_t ze_rank_info_vec;

    bool is_p2p_access_enabled = false;
    port_health_status port_status = port_health_status::unknown;
#endif // CCL_ENABLE_SYCL && CCL_ENABLE_ZE
};

} // namespace ccl
