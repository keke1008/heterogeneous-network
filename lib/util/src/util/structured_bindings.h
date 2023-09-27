#pragma once

#include <etl/array.h>

// definition
namespace std {
    template <typename T>
    struct tuple_size;

    template <size_t I, typename T>
    class tuple_element;
} // namespace std

// etl::array<T, N>へのpolyfill
namespace std {
    template <typename T, size_t N>
    struct tuple_size<etl::array<T, N>> : etl::integral_constant<size_t, N> {};

    template <size_t I, typename T, size_t N>
    struct tuple_element<I, etl::array<T, N>> {
        using type = T;
    };

    template <size_t I, typename T, size_t N>
    struct tuple_element<I, const etl::array<T, N>> {
        using type = const T;
    };
} // namespace std
