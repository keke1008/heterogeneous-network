#pragma once

#include <etl/type_traits.h>
#include <stddef.h>

namespace tl {
    template <size_t N, typename... Ts>
    struct type_index;

    template <size_t N, typename T, typename... Ts>
    struct type_index<N, T, Ts...> : type_index<N - 1, Ts...> {};

    template <typename T, typename... Ts>
    struct type_index<0, T, Ts...> {
        using type = T;
    };

    template <size_t N, typename... Ts>
    using type_index_t = typename type_index<N, Ts...>::type;

    template <typename T, typename... Ts>
    struct index_of;

    template <typename T, typename... Ts>
    struct index_of<T, T, Ts...> : etl::integral_constant<size_t, 0> {};

    template <typename T, typename U, typename... Ts>
    struct index_of<T, U, Ts...> : etl::integral_constant<size_t, 1 + index_of<T, Ts...>::value> {};

    template <typename T, typename... Ts>
    constexpr size_t index_of_v = index_of<T, Ts...>::value;
} // namespace tl
