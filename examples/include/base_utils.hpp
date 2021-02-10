#pragma once

#include <algorithm>
#include <iterator>
#include <sstream>
#include <tuple>
#include <utility>

template <int CurIndex, class T, class U, class... Args>
struct get_tuple_elem_index {
    static constexpr int index = get_tuple_elem_index<CurIndex + 1, T, Args...>::index;
};

template <int CurIndex, class T, class... Args>
struct get_tuple_elem_index<CurIndex, T, T, Args...> {
    static constexpr int index = CurIndex;
};

template <class T, class... Args>
typename std::remove_reference<typename std::remove_cv<T>::type>::type& ccl_tuple_get(
    std::tuple<Args...>& t) {
    using non_cv_type = typename std::remove_cv<T>::type;
    using non_ref_type = typename std::remove_reference<non_cv_type>::type;
    return std::get<get_tuple_elem_index<0, non_ref_type, Args...>::index>(t);
}

template <class T, class... Args>
const typename std::remove_reference<typename std::remove_cv<T>::type>::type& ccl_tuple_get(
    const std::tuple<Args...>& t) {
    using non_cv_type = typename std::remove_cv<T>::type;
    using non_ref_type = typename std::remove_reference<non_cv_type>::type;
    return std::get<get_tuple_elem_index<0, non_ref_type, Args...>::index>(t);
}

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

namespace utils {

template <typename T>
void str_to_array(const char* input, std::vector<T>& output, char delimiter) {
    if (!input) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.push_back(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}
template <>
void str_to_array(const char* input, std::vector<std::string>& output, char delimiter) {
    std::string processes_input(input);

    processes_input.erase(std::remove_if(processes_input.begin(),
                                         processes_input.end(),
                                         [](unsigned char x) {
                                             return std::isspace(x);
                                         }),
                          processes_input.end());

    std::replace(processes_input.begin(), processes_input.end(), delimiter, ' ');
    std::stringstream ss(processes_input);

    while (ss >> processes_input) {
        output.push_back(processes_input);
    }
}

template <typename T>
void str_to_mset(const char* input, std::multiset<T>& output, char delimiter) {
    if (!input) {
        return;
    }
    std::stringstream ss(input);
    T temp{};
    while (ss >> temp) {
        output.insert(temp);
        if (ss.peek() == delimiter) {
            ss.ignore();
        }
    }
}
} // namespace utils
