#pragma once
#include "scaleup_ctx.hpp"

namespace native
{

class ccl_gpu_comm;
class ccl_virtual_gpu_comm;

template <class ctx_impl_t>
using scale_up_ctx_specific = scaleup_ctx_base<ctx_impl_t, ccl_gpu_comm, ccl_virtual_gpu_comm>;

}
