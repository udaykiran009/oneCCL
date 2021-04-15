#include "coll/coll_param.hpp"

bool operator==(const coll_param_gpu& lhs, const coll_param_gpu& rhs) {
    assert(lhs.is_reduction() && rhs.is_reduction() ||
           (!lhs.is_reduction() && !rhs.is_reduction()));

    bool res =
        lhs.get_coll_type() == rhs.get_coll_type() && lhs.get_datatype() == rhs.get_datatype();

    if (lhs.is_reduction()) {
        res = res && lhs.get_reduction() == rhs.get_reduction();
    }

    return res;
}