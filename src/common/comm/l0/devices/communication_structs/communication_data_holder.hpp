#pragma once
#include "include/oneapi/ccl/types.hpp"
#include "coll/algorithm/algorithms_enum.hpp"

namespace native {

template <class... T>
struct communiaction_data_holder {
    template <class U, ccl_coll_type type>
    struct data_for_algo_t {
        U data;
    };

    template <class Data, ccl_coll_type... types>
    using data_storage_t = std::tuple<data_for_algo_t<Data, types>...>;
};
} // namespace native
CCL_COLL_TYPE_LIST
