#pragma once
#include "coll/algorithms/algorithms_enum.hpp"

template <class type>
struct kernel_params_default {
    using native_type = type;
};

template <class native_data_type, ccl_coll_reduction reduction>
struct kernel_reduction_params_traits : kernel_params_default<native_data_type> {
    using typename kernel_params_default<native_data_type>::native_type;
    static constexpr ccl_coll_reduction red_type = reduction;
};
