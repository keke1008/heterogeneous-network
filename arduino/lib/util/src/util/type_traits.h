#pragma once

#include <etl/utility.h>

namespace util {
    namespace {
        template <typename T, typename = void>
        struct invoke_result_impl {};

        template <typename F, typename... Args>
        struct invoke_result_impl<
            F(Args...),
            etl::void_t<decltype(etl::declval<F>()(etl::declval<Args>()...))>> {
            using type = decltype(etl::declval<F>()(etl::declval<Args>()...));
        };

    } // namespace

    template <typename F, typename... Args>
    struct invoke_result : invoke_result_impl<F && (Args && ...)> {};

    template <typename F, typename... Args>
    using invoke_result_t = typename invoke_result<F, Args...>::type;

    namespace {
        template <typename T, typename = void>
        struct is_invocable_impl : etl::false_type {};

        template <typename F, typename... Args>
        struct is_invocable_impl<
            F(Args...),
            etl::void_t<decltype(etl::declval<F>()(etl::declval<Args>()...))>> : etl::true_type {};
    } // namespace

    template <typename F, typename... Args>
    struct is_invocable : is_invocable_impl<F && (Args && ...)> {};

    template <typename F, typename... Args>
    inline constexpr bool is_invocable_v = is_invocable<F, Args...>::value;
} // namespace util
