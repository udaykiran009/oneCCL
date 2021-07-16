#pragma once

#include <tuple>

namespace ccl {
namespace utils {

inline size_t calculate_hash(size_t left, size_t right) {
    left = ((left >> 2) + right + (left << 6) + 0x9e3779b9) ^ left;
    return left;
}

template <size_t idx, class... types>
struct hash_impl {
    size_t operator()(size_t left, const std::tuple<types...>& tuple) const {
        using next_t = typename std::tuple_element<idx, std::tuple<types...>>::type;
        hash_impl<idx - 1, types...> next;
        size_t right = std::hash<next_t>()(std::get<idx>(tuple));
        return next(calculate_hash(left, right), tuple);
    }
};

template <class... types>
struct hash_impl<0, types...> {
    size_t operator()(size_t left, const std::tuple<types...>& tuple) const {
        using next_t = typename std::tuple_element<0, std::tuple<types...>>::type;
        size_t right = std::hash<next_t>()(std::get<0>(tuple));
        return calculate_hash(left, right);
    }
};

struct tuple_hash {
    template <class... types>
    size_t operator()(const std::tuple<types...>& tuple) const {
        const size_t start = std::tuple_size<std::tuple<types...>>::value - 1;
        return hash_impl<start, types...>()(0, tuple);
    }
};

} // namespace utils
} // namespace ccl
