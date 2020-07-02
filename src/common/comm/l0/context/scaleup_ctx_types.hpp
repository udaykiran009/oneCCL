#pragma once
#include "base_scaling_ctx.hpp"

namespace native
{

class ccl_gpu_comm;
class ccl_virtual_gpu_comm;

template <class ctx_impl_t>
using scale_up_ctx_specific = base_scaling_ctx<ctx_impl_t, ccl_gpu_comm, ccl_virtual_gpu_comm>;

class numa_ctx : public base_scaling_ctx<numa_ctx, ccl_gpu_comm, ccl_virtual_gpu_comm>
{
}
}
