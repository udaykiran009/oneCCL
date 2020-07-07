#pragma once

namespace ccl
{
template<class impl_t>
class non_copyable
{
public:
    non_copyable(const non_copyable&) = delete;
    impl_t& operator= (const impl_t&) = delete;

protected:
    non_copyable() = default;
    ~non_copyable() = default;
};

template<class impl_t>
class non_movable
{
public:
    non_movable(non_movable&&) = delete;
    impl_t& operator= (impl_t&&) = delete;

protected:
    non_movable() = default;
    ~non_movable() = default;
};
}
