#pragma once

#include <etl/optional.h>
#include <etl/type_traits.h>
#include <stdint.h>

namespace nb::stream {
    using etl::declval, etl::is_same;

    template <typename T, typename Item>
    using is_stream_reader = etl::conjunction<
        is_same<typename T::ReadableStreamItem, Item>,
        is_same<decltype(declval<T>().readable_count()), size_t>,
        is_same<decltype(declval<T>().read()), etl::optional<Item>>,
        is_same<decltype(declval<T>().is_closed()), bool>>;
    template <typename T, typename Item>
    inline constexpr bool is_stream_reader_v = is_stream_reader<T, Item>::value;

    template <typename T, typename Item>
    using is_stream_writer = etl::conjunction<
        is_same<typename T::WritableStreamItem, Item>,
        is_same<decltype(declval<T>().writable_count()), size_t>,
        is_same<decltype(declval<T>().write(declval<Item>())), bool>,
        is_same<decltype(declval<T>().is_closed()), bool>>;
    template <typename T, typename Item>
    inline constexpr bool is_stream_writer_v = is_stream_writer<T, Item>::value;
} // namespace nb::stream
