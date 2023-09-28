#pragma once

#include <etl/type_traits.h>
#include <etl/utility.h>
#include <util/type_traits.h>

namespace util {
    // 型の関係
    template <typename T, typename U>
    concept same_as = etl::is_same_v<T, U> && etl::is_same_v<U, T>;

    template <typename From, typename To>
    concept convertible_to =
        etl::is_convertible_v<From, To> &&
        requires(etl::add_rvalue_reference_t<From> (&f)()) { static_cast<To>(f()); };

    // 型の種類
    template <typename T>
    concept integral = etl::is_integral_v<T>;

    template <typename T>
    concept signed_integral = integral<T> && etl::is_signed_v<T>;

    template <typename T>
    concept unsigned_integral = integral<T> && !signed_integral<T>;

    template <typename T>
    concept floating_point = etl::is_floating_point_v<T>;

    // 比較
    template <typename T, typename U>
    concept equality_comparable_with =
        requires(const etl::remove_reference_t<T> &t, const etl::remove_reference_t<U> &u) {
            { t == u } -> same_as<bool>;
            { t != u } -> same_as<bool>;
            { u == t } -> same_as<bool>;
            { u != t } -> same_as<bool>;
        };

    template <typename T>
    concept equality_comparable = equality_comparable_with<T, T>;

    template <typename T, typename U>
    concept totally_ordered_with =
        equality_comparable_with<T, U> &&
        requires(const etl::remove_reference_t<T> &t, const etl::remove_reference_t<U> &u) {
            { t < u } -> same_as<bool>;
            { t > u } -> same_as<bool>;
            { t <= u } -> same_as<bool>;
            { t >= u } -> same_as<bool>;
            { u < t } -> same_as<bool>;
            { u > t } -> same_as<bool>;
            { u <= t } -> same_as<bool>;
            { u >= t } -> same_as<bool>;
        };

    template <typename T>
    concept totally_ordered = totally_ordered_with<T, T>;

    // 関数呼び出し
    template <typename F, typename... Args>
    concept invocable = util::is_invocable_v<F, Args...>;
} // namespace util
