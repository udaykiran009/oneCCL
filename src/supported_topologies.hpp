#pragma once

#include "common/utils/enums.hpp"
#include "internal_types.hpp"

inline std::string to_string(ccl::group_split_type type) {
    using device_group_split_type_names = ::utils::enum_to_str<
        static_cast<typename std::underlying_type<ccl::group_split_type>::type>(
            ccl::group_split_type::last_value)>;
    return device_group_split_type_names({
                                             "TG",
                                             "PG",
                                             "CG",
                                         })
        .choose(type, "INVALID_VALUE");
}
