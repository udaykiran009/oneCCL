#pragma once

#include "atl/atl_base_comm.hpp"
#include "oneapi/ccl/config.h"

#ifdef CCL_ENABLE_MPI
#include <mpi.h>
#endif // CCL_ENABLE_MPI

#include <algorithm>
#include <numeric>
#include <set>
#include <string>

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include "common/ze/ze_api_wrapper.hpp"
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

namespace ccl {

enum class topo_color_mode : int { fixed, ze };
static std::map<topo_color_mode, std::string> topo_color_names = {
    std::make_pair(topo_color_mode::fixed, "fixed"),
    std::make_pair(topo_color_mode::ze, "ze")
};

static constexpr int topo_uuid_len = 35;

struct topo_rank_info {
    int rank;
    int host_idx;
    char uuid[topo_uuid_len];

    topo_rank_info();
};

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
struct topo_ze_rank_info {
    ze_device_uuid_t device_uuid{};
    zes_pci_address_t pci_addr{};
};

struct topo_ze_port_info {
    zes_fabric_port_id_t local{};
    zes_fabric_port_id_t remote{};
};
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

class topo_manager {
public:
    static constexpr int invalid_color = -1;
    static constexpr int invalid_host_idx = -1;
    static constexpr int invalid_plane_idx = -1;
    static constexpr int max_hostname_len = 256;
    static constexpr int max_ranks_per_host = 1000;
    static constexpr int max_ranks_per_card = 2;
    static constexpr int max_ranks_per_plane = 8;

    topo_manager() = default;
    topo_manager(const topo_manager& other) = default;
    topo_manager& operator=(const topo_manager& other) = default;

    ~topo_manager() = default;

    void init(std::shared_ptr<atl_base_comm> atl_comm,
              std::shared_ptr<ccl::device> device_ptr,
              std::shared_ptr<ccl::context> context_ptr);

    int get_intra_card_color(int rank) const;
    int get_inter_card_color(int rank) const;
    std::string get_uuid(int rank) const;

    bool has_p2p_access() const;

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    static bool is_same_pci_addr(zes_pci_address_t addr1, zes_pci_address_t addr2);
    static std::vector<std::vector<bool>> build_p2p_matrix(
        const std::vector<ze_device_handle_t>& devices);
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

    std::string to_string();

    bool is_single_node = false;
    bool is_single_card = false;

private:
    void base_init(std::shared_ptr<atl_base_comm> atl_comm,
                   std::shared_ptr<ccl::device> device_ptr,
                   std::shared_ptr<ccl::context> context_ptr);
    void post_init();

    void allgather(const void* send_buf, void* recv_buf, int bytes);
    void allgatherv(const void* send_buf, void* recv_buf, const std::vector<int>& recv_bytes);

    static std::string generate_uuid();

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    ze_device_handle_t device{};
    static std::string to_string(const std::vector<std::vector<bool>>& p2p_matrix);
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

    std::shared_ptr<atl_base_comm> comm;

    int host_idx = invalid_host_idx;
    std::vector<int> intra_card_colors{};
    std::vector<int> inter_card_colors{};
    std::vector<std::string> uuids{};
    std::vector<topo_rank_info> rank_info_vec;
    std::vector<std::vector<bool>> p2p_matrix;
};

} // namespace ccl
