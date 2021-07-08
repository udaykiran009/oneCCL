#pragma once

#include <string>

#include "coll/coll_param.hpp"

namespace native {
namespace detail {

std::string to_string(ccl::reduction red);
std::string get_kernel_name(const std::string& kernel_name, const coll_param_gpu& params);

} // namespace detail
} // namespace native