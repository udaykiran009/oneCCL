#pragma once
#include "common/log/log.hpp"

namespace ccl {

class topo_manager {
public:
    topo_manager() = default;
    topo_manager(const topo_manager& other) = default;
    topo_manager& operator=(const topo_manager& other) = default;

    ~topo_manager() = default;

    void init();
    int get_intra_card_color(int rank);
    int get_inter_card_color(int rank);
};

} // namespace ccl
