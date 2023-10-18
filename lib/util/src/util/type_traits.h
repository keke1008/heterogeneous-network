#pragma once

#include <etl/utility.h>

namespace util {
    template <typename F, typename... Args>
    struct invoke_result {
        using type = decltype(etl::declval<F>()(etl::declval<Args>()...));
    };

    template <typename F, typename... Args>
    using invoke_result_t = typename invoke_result<F, Args...>::type;

    template <typename F, typename... Args>
    struct is_invocable {
        template <typename U>
        static constexpr auto test(U *p)
            -> decltype((*p)(etl::declval<Args>()...), etl::true_type{});

        template <typename U>
        static constexpr etl::false_type test(...);

        static constexpr bool value = decltype(test<F>(nullptr))::value;
    };

    template <typename F, typename... Args>
    inline constexpr bool is_invocable_v = is_invocable<F, Args...>::value;
} // namespace util
