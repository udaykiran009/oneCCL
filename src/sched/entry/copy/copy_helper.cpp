#include "sched/entry/copy/copy_helper.hpp"

const int copy_helper::invalid_rank = -1;

copy_attr::copy_attr(int peer_rank,
                     size_t peer_buf_idx,
                     copy_direction direction,
                     size_t in_buf_offset)
        : peer_rank(peer_rank),
          peer_buf_idx(peer_buf_idx),
          direction(direction),
          in_buf_offset(in_buf_offset) {}

copy_attr::copy_attr(int peer_rank, copy_direction direction, size_t in_buf_offset)
        : peer_rank(peer_rank),
          peer_buf_idx(0),
          direction(direction),
          in_buf_offset(in_buf_offset) {}

using copy_direction_str_enum =
    utils::enum_to_str<utils::enum_to_underlying(copy_direction::d2d) + 1>;
std::string to_string(copy_direction val) {
    return copy_direction_str_enum({ "UNDEFINED", "H2H", "D2H", "H2D", "D2D" })
        .choose(val, "UNKNOWN");
}
