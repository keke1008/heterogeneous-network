#pragma once

#include <etl/optional.h>
#include <etl/type_traits.h>
#include <stdint.h>

namespace nb::stream {
    using etl::declval, etl::is_same, etl::is_convertible;

    // StreamReader

    template <typename T>
    using has_stream_reader_members = etl::conjunction<
        is_same<decltype(declval<const T>().is_readable()), bool>,
        is_convertible<decltype(declval<const T>().readable_count()), size_t>,
        is_convertible<decltype(declval<const T>().readable_count()), size_t>,
        is_same<decltype(declval<const T>().is_reader_closed()), bool>>;

    template <typename T, typename = void>
    struct is_stream_reader : etl::false_type {};

    template <typename T>
    struct is_stream_reader<T, etl::void_t<has_stream_reader_members<etl::decay_t<T>>>>
        : has_stream_reader_members<etl::decay_t<T>> {};

    template <typename T>
    using is_stream_reader_t = typename is_stream_reader<T>::type;
    template <typename T>
    inline constexpr bool is_stream_reader_v = is_stream_reader<T>::value;

    template <typename... Ts>
    using are_all_stream_reader = etl::conjunction<is_stream_reader_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_stream_reader_v = are_all_stream_reader<Ts...>::value;

    // StreamWriter
    template <typename T>
    using has_stream_writer_members = etl::conjunction<
        is_same<decltype(declval<const T>().is_writable()), bool>,
        is_convertible<decltype(declval<const T>().writable_count()), size_t>,
        is_same<decltype(declval<T>().write(declval<uint8_t>())), bool>,
        is_same<decltype(declval<const T>().is_writer_closed()), bool>>;

    template <typename T, typename = void>
    struct is_stream_writer : etl::false_type {};

    template <typename T>
    struct is_stream_writer<T, etl::void_t<has_stream_writer_members<etl::decay_t<T>>>>
        : has_stream_writer_members<etl::decay_t<T>> {};

    template <typename T>
    using is_stream_writer_t = typename is_stream_writer<T>::type;
    template <typename T>
    inline constexpr bool is_stream_writer_v = is_stream_writer<T>::value;

    template <typename... Ts>
    using are_all_stream_writer = etl::conjunction<is_stream_writer_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_stream_writer_v = are_all_stream_writer<Ts...>::value;
} // namespace nb::stream
