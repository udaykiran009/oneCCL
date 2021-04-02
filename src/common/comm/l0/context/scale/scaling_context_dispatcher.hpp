#pragma once
#include <tuple>
#include <memory>
#include <stdexcept>
#include <vector>
#include <type_traits>
#include "common/comm/l0/devices/proxy_observer.hpp"

namespace native {

template <class Device, class... Others>
struct ctx_resolver {};

template <class Device, class T, class... Ctx>
struct ctx_resolver<Device, T, Ctx...> {
    using type = typename std::conditional<T::template is_registered_device_t<Device>(),
                                           T,
                                           typename ctx_resolver<Device, Ctx...>::type>::type;
};

struct null_ctx_t {};

template <class Device, class T>
struct ctx_resolver<Device, T> {
    using type = typename std::
        conditional<T::template is_registered_device_t<Device>(), T, null_ctx_t>::type;
};

template <class... Contexts>
struct scaling_ctx_dispatcher : public Contexts... {
    template <class device_t>
    typename std::add_pointer<typename ctx_resolver<device_t, Contexts...>::type>::type
    dispatch_context() {
        using resolved_ctx_t = typename ctx_resolver<device_t, Contexts...>::type;

        static_assert(not std::is_same<resolved_ctx_t, null_ctx_t>::value,
                      "Not found scaling context type for requested `device_t`");

        using scaling_ctx_pointer_t = typename std::add_pointer<resolved_ctx_t>::type;
        return static_cast<scaling_ctx_pointer_t>(this);
    }
};

} //namespace native
