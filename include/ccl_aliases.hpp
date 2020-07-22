#pragma once

#include <array>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

namespace ccl
{
template <class T, class Alloc = std::allocator<T>>
using vector_class = std::vector<T, Alloc>;

template <class T, std::size_t N>
using array_class = std::array<T, N>;

using string_class = std::string;

template <class R, class... ArgTypes>
using function_class = std::function<R(ArgTypes...)>;

using mutex_class = std::mutex;

template <class T1, class T2>
using pair_class = std::pair<T1, T2>;

template <class... Types>
using tuple_class = std::tuple<Types...>;

template <class T>
using shared_ptr_class = std::shared_ptr<T>;

template <class T>
using unique_ptr_class = std::unique_ptr<T>;
}
