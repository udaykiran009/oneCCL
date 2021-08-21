#pragma once

#include "coll/coll_param.hpp"

#ifdef CCL_ENABLE_SYCL
#include <CL/sycl.hpp>
#endif // CCL_ENABLE_SYCL

#ifdef CCL_ENABLE_SYCL
void ccl_check_usm_pointers(const std::vector<void*>& ptrs,
                            const sycl::device& dev,
                            const sycl::context& ctx);
#endif // CCL_ENABLE_SYCL

void ccl_coll_validate_user_input(const ccl_coll_param& param, const ccl_coll_attr& attr);
