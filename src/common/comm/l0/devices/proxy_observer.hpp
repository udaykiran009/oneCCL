#pragma once
#include "common/comm/l0/device_types.hpp"

namespace native
{

template<class device_t>
class proxy_observer
{
public:

    using impl = device_t;
    using type_idx_t = typename std::underlying_type<gpu_types>::type;

    static constexpr type_idx_t idx()
    {
        return impl::type_idx() - gpu_types::SCALE_UP_GPU;
    }

    template <class ...Args>
    void notify(Args&&... args)
    {
        get_this()->notify_impl(std::forward<Args>(args)...);
    }

    impl* get_this()
    {
        return static_cast<device_t*>(this);
    }

    const impl* get_this() const
    {
        return static_cast<const impl*>(this);
    }
private:
};
}
