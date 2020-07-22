#pragma once

namespace ccl {
template <class impl_t>
class non_copyable {
public:
    non_copyable(const non_copyable&) = delete;
    impl_t& operator=(const impl_t&) = delete;

protected:
    non_copyable() = default;
    ~non_copyable() = default;
};

template <class impl_t>
class non_movable {
public:
    non_movable(non_movable&&) = delete;
    impl_t& operator=(impl_t&&) = delete;

protected:
    non_movable() = default;
    ~non_movable() = default;
};

template <class derived_t, class impl_t>
class pointer_on_impl {
protected:
    using impl_value_t = std::unique_ptr<impl_t>;
    using parent_t = derived_t;

    pointer_on_impl(impl_value_t&& impl) : pimpl(std::move(impl)) {}
    ~pointer_on_impl() = default;

    impl_t& get_impl() {
        return *pimpl; }
    const impl_t& get_impl() const {
        return *pimpl; }
private:
    impl_value_t pimpl;
};
} // namespace ccl
