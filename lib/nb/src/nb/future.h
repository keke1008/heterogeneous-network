#pragma once

#include "./poll.h"
#include <etl/type_traits.h>

namespace nb {
    template <typename TFuture, typename TReturn>
    using has_future_members =
        etl::is_same<decltype(etl::declval<TFuture>().poll()), Poll<TReturn>>;

    template <typename Future, typename Return, typename = void>
    struct is_future : etl::false_type {};

    template <typename TFuture, typename TReturn>
    struct is_future<TFuture, TReturn, etl::void_t<has_future_members<TFuture, TReturn>>>
        : has_future_members<TFuture, TReturn> {};

    template <typename TFuture, typename TReturn>
    constexpr bool is_future_v = is_future<TFuture, TReturn>::value;
}; // namespace nb
