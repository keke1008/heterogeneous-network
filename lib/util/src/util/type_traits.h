#pragma once

#include <etl/utility.h>

namespace util {
    template <typename F, typename... Args>
    struct invoke_result {
        using type = decltype(etl::declval<F>()(etl::declval<Args>()...));
    };

    template <typename F, typename... Args>
    using invoke_result_t = typename invoke_result<F, Args...>::type;
} // namespace util
