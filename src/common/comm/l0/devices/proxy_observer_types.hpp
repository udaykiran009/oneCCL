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
    template<ccl::device_topology_type topology_type, class ctx_t>
    void assign(ctx_t& ctx)
    {
        // context linkage
        ctx.template attach<topology_type>(this);
        std::get<topology_type>(contexts) = &ctx;
    }

    template<ccl::device_topology_type topology_type>
    void invoke()
    {
        //use context to invoke/register proxy jobs
        std::get<topology_type>(contexts).invoke_proxy(this);
    }
};


template <class impl_t>
using proxy_observer_specific = proxy_multiple_observer<impl_t, device_group_context,
                                                                device_group_context,
                                                                thread_group_context,
                                                                thread_group_context,
                                                                process_group_context,
                                                                process_group_context,

                                                                device_group_context,
                                                                thread_group_context,
                                                                process_group_context
                                                                >;
}
