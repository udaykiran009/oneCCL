#ifndef BASE_UTILS_HPP
#define BASE_UTILS_HPP

#include <iterator>
#include <utility>
#include <tuple>

template <class specific_tuple, class functor, size_t cur_index>
void ccl_tuple_for_each_impl(specific_tuple&& t, functor f, std::true_type tuple_finished) {
    // nothing to do
}

template <class specific_tuple, class functor, size_t cur_index>
void ccl_tuple_for_each_impl(specific_tuple&& t, functor f, std::false_type tuple_not_finished) {
    f(std::get<cur_index>(std::forward<specific_tuple>(t)));

    constexpr std::size_t tuple_size =
        std::tuple_size<typename std::remove_reference<specific_tuple>::type>::value;

    using is_tuple_finished_t = std::integral_constant<bool, cur_index + 1 >= tuple_size>;

    ccl_tuple_for_each_impl<specific_tuple, functor, cur_index + 1>(
        std::forward<specific_tuple>(t), f, is_tuple_finished_t{});
}

template <class specific_tuple, class functor, size_t cur_index = 0>
void ccl_tuple_for_each(specific_tuple&& t, functor f) {
    constexpr std::size_t tuple_size =
        std::tuple_size<typename std::remove_reference<specific_tuple>::type>::value;
    static_assert(tuple_size != 0, "Nothing to do, tuple is empty");

    using is_tuple_finished_t = std::integral_constant<bool, cur_index >= tuple_size>;
    ccl_tuple_for_each_impl<specific_tuple, functor, cur_index>(
        std::forward<specific_tuple>(t), f, is_tuple_finished_t{});
}

template <typename specific_tuple, size_t cur_index, typename functor, class... FunctionArgs>
void ccl_tuple_for_each_indexed_impl(functor,
                                     std::true_type tuple_finished,
                                     const FunctionArgs&... args) {}

template <typename specific_tuple, size_t cur_index, typename functor, class... FunctionArgs>
void ccl_tuple_for_each_indexed_impl(functor f,
                                     std::false_type tuple_not_finished,
                                     const FunctionArgs&... args) {
    using tuple_element_t = typename std::tuple_element<cur_index, specific_tuple>::type;

    f.template invoke<cur_index, tuple_element_t>(args...);

    constexpr std::size_t tuple_size =
        std::tuple_size<typename std::remove_reference<specific_tuple>::type>::value;

    using is_tuple_finished_t = std::integral_constant<bool, cur_index + 1 >= tuple_size>;

    ccl_tuple_for_each_indexed_impl<specific_tuple, cur_index + 1, functor>(
        f, is_tuple_finished_t{}, args...);
}

template <typename specific_tuple, typename functor, class... FunctionArgs>
void ccl_tuple_for_each_indexed(functor f, const FunctionArgs&... args) {
    constexpr std::size_t tuple_size =
        std::tuple_size<typename std::remove_reference<specific_tuple>::type>::value;
    static_assert(tuple_size != 0, "Nothing to do, tuple is empty");

    using is_tuple_finished_t = std::false_type; //non-empty tuple started
    ccl_tuple_for_each_indexed_impl<specific_tuple, 0, functor, FunctionArgs...>(
        f, is_tuple_finished_t{}, args...);
}

#endif /* BASE_UTILS_HPP */
