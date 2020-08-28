#pragma once

#include "proxy_observer.hpp"

namespace native
{
struct device_group_context;
struct thread_group_context;
struct process_group_context;


template<class impl, class ...context_t>
class proxy_multiple_observer : public proxy_observer<impl>
{
    using registered_contexts = std::tuple<typename std::add_pointer<context_t>::type ...>;
    registered_contexts contexts;

public:

    template<ccl::device_group_split_type group_id,
             ccl::device_topology_type class_id,
             class ctx_reg_t,
             class ctx_t>
    void assign(ctx_reg_t& ctx_to_reg, ctx_t ctx)
    {
        // context linkage
        ctx.template attach<group_id, class_id>(static_cast<impl*>(this));
        std::get<utils::enum_to_underlying(group_id)>(contexts) = &ctx_to_reg;
    }

    template<ccl::device_group_split_type group_id,
             ccl::device_topology_type class_id>
    void invoke()
    {
        //use context to invoke/register proxy jobs
        std::get<utils::enum_to_underlying(group_id)>(contexts).invoke_proxy(static_cast<impl*>(this));
    }
};


template <class impl_t>
using proxy_observer_specific = proxy_multiple_observer<impl_t, device_group_context,
                                                                thread_group_context,
                                                                process_group_context
/*
                                                                device_group_context,
                                                                thread_group_context,
                                                                process_group_context
 */                                                               >;
}
