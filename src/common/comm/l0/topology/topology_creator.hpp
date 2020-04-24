#pragma once
namespace native
{
namespace details
{

template<class F>
struct device_group_container_functor
{
    template<class ...Args>
    device_group_container_functor(Args&& ...args):
        operation(std::forward<Args>(args)...)
    {
    }

    template<class device_container_t>
    void operator() (device_container_t& container)
    {
        operation(container);
    }
    F& get_functor()
    {
        return operation;
    }
private:
    F operation;
};
}

template<class F, class ...Args>
details::device_group_container_functor<F> create_device_functor(Args&& ...args)
{
    return details::device_group_container_functor<F> (std::forward<Args>(args)...);
}
}
