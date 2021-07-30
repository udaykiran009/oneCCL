#include "sched/entry/copy/copy_helper.hpp"

using copy_direction_str_enum =
    utils::enum_to_str<utils::enum_to_underlying(copy_direction::d2d) + 1>;
std::string to_string(copy_direction val) {
    return copy_direction_str_enum({ "D2H", "H2D", "D2D" }).choose(val, "UNKNOWN");
}

const int copy_helper::invalid_rank = -1;