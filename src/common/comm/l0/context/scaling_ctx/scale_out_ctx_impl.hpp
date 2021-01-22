#pragma once
#include "common/comm/l0/context/scaling_ctx/scale_out_ctx.hpp"
#include "common/log/log.hpp"

namespace native {

#define TEMPLATE_DECL_ARG class Impl, ccl::device_topology_type... types
#define TEMPLATE_DEF_ARG  Impl, types...

// observer_ptr interface implementations
template <TEMPLATE_DECL_ARG>
template <ccl::device_topology_type topology_type, class device_t>
void scale_out_ctx<TEMPLATE_DEF_ARG>::register_observer_impl(size_t rank_addr,
                                                             observer_t<device_t>* observer_ptr) {
    observer::container_t<observer_t<device_t>>& container =
        scaling_ctx_base_t::template get_types_container<observer_t<device_t>, topology_type>(
            observables);
    container.insert(observer_ptr);
}

#undef TEMPLATE_DECL_ARG
#undef TEMPLATE_DEF_ARG
} // namespace native
