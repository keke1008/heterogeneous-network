#pragma once

#include <etl/utility.h>

namespace tl {
    template <typename... Ts>
    struct Visitor : Ts... {
        using Ts::operator()...;

        template <typename... Args>
        using ReturnType = decltype(etl::declval<Visitor<Ts...>>()(etl::declval<Args>()...));
    };
    template <class... Ts>
    Visitor(Ts...) -> Visitor<Ts...>;
} // namespace tl
