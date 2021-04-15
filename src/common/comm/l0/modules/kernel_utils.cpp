#include "common/comm/l0/modules/kernel_utils.hpp"
#include "common/global/global.hpp"

namespace native {
namespace detail {

std::string to_string(ccl::reduction red) {
#define P(val) \
    case ccl::reduction::val: return #val;

    switch (red) {
        P(sum);
        P(prod);
        P(min);
        P(max);
        default:
            throw std::runtime_error("Unexpected value of reduction: " +
                                     std::to_string(static_cast<int>(red)));
    }

#undef P
}

// TODO: ideally we should take a set of all parameters and generate a kernel name
// to execute
std::string get_kernel_name(const std::string& kernel_name, const coll_param_gpu& params) {
    // TODO: introduce a simple function to map names?
    // Can we remove dtypes from global_data then? Do we need custom datatypes?
    auto name = kernel_name + "_" + ccl::global_data::get().dtypes->name(params.get_datatype());
    if (params.is_reduction()) {
        name += "_" + to_string(params.get_reduction());
    }

    return name;
}

} // namespace detail
} // namespace native