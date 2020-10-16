#include "sched/entry/sycl_entry_helper.hpp"

using sycl_copy_direction_str_enum =
    utils::enum_to_str<utils::enum_to_underlying(sycl_copy_direction::h2d) + 1>;
std::string to_string(sycl_copy_direction val) {
    return sycl_copy_direction_str_enum(
        { "D2H",
          "H2D" }).choose(val, "UNKNOWN");
}
