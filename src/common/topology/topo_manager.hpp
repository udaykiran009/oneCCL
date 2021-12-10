#pragma once

#include "atl/atl_base_comm.hpp"
#include "oneapi/ccl/config.h"

#ifdef CCL_ENABLE_MPI
#include <mpi.h>
#endif // CCL_ENABLE_MPI

#include <set>
#include <string>

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
#include <ze_api.h>
#include <zes_api.h> // for pci addr
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

namespace ccl {

struct rank_topo_info {
    int rank;
#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    ze_device_uuid_t uuid{};
    zes_pci_address_t pci_addr{};
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

    rank_topo_info();
};

enum class topo_color_mode : int { fixed, ze };
static std::map<topo_color_mode, std::string> topo_color_names = {
    std::make_pair(topo_color_mode::fixed, "fixed"),
    std::make_pair(topo_color_mode::ze, "ze")
};

class topo_manager {
public:
    static constexpr int invalid_color = -1;
    static constexpr int invalid_host_idx = -1;
    static constexpr int max_hostname_len = 256;
    static constexpr int max_ranks_per_host = 1000;

    topo_manager() = default;
    topo_manager(const topo_manager& other) = default;
    topo_manager& operator=(const topo_manager& other) = default;

    ~topo_manager() = default;

    void init(std::shared_ptr<atl_base_comm> atl_comm,
              std::shared_ptr<ccl::device> device_ptr,
              std::shared_ptr<ccl::context> context_ptr);

    int get_intra_card_color(int rank);
    int get_inter_card_color(int rank);

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    static bool is_same_pci_addr(zes_pci_address_t addr1, zes_pci_address_t addr2);
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL

    std::string to_string();

private:
    int host_idx = invalid_host_idx;
    std::vector<int> intra_colors{};
    std::vector<int> inter_colors{};

#if defined(CCL_ENABLE_ZE) && defined(CCL_ENABLE_SYCL)
    ze_device_handle_t device{};
#endif // CCL_ENABLE_ZE && CCL_ENABLE_SYCL
};

} // namespace ccl
