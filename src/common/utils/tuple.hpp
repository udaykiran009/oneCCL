#pragma once

#include <utility>
#include <tuple>

template<typename TupleType, typename FunctionType>
void ccl_tuple_for_each(TupleType&&, FunctionType,
                        std::integral_constant<size_t, std::tuple_size<typename std::remove_reference<TupleType>::type >::value>)
{}

template<std::size_t I, typename TupleType, typename FunctionType
       , typename = typename std::enable_if<I!=std::tuple_size<typename std::remove_reference<TupleType>::type>::value>::type >
void ccl_tuple_for_each(TupleType&& t, FunctionType f, std::integral_constant<size_t, I>)
{
    f(std::get<I>(std::forward<TupleType>(t)));
    ccl_tuple_for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, I + 1>());
}

template<typename TupleType, typename FunctionType>
void ccl_tuple_for_each(TupleType&& t, FunctionType f)
{
    ccl_tuple_for_each(std::forward<TupleType>(t), f, std::integral_constant<size_t, 0>());
}
