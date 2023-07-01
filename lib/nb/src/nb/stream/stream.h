#pragma once

#include <etl/optional.h>
#include <etl/type_traits.h>
#include <stdint.h>

namespace nb::stream {
    using etl::declval, etl::is_same;

    // StreamReader
    template <typename T>
    using reader_item = typename etl::remove_reference_t<T>::StreamReaderItem;
    template <typename... Ts>
    using readers_item = etl::enable_if_t<
        etl::are_all_same_v<reader_item<Ts>...>,
        etl::common_type_t<reader_item<Ts>...>>;

    template <typename T>
    using stream_reader_type = etl::conjunction<
        is_same<decltype(declval<T>().read()), etl::optional<reader_item<T>>>,
        is_same<decltype(declval<T>().is_readable()), bool>>;

    template <typename T, typename = void>
    struct is_stream_reader : etl::false_type {};

    template <typename T>
    struct is_stream_reader<T, etl::void_t<stream_reader_type<T>>> : stream_reader_type<T> {};

    template <typename T>
    using is_stream_reader_t = typename is_stream_reader<T>::type;
    template <typename T>
    inline constexpr bool is_stream_reader_v = is_stream_reader<T>::value;

    template <typename... Ts>
    using are_all_stream_reader = etl::conjunction<is_stream_reader_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_stream_reader_v = are_all_stream_reader<Ts...>::value;

    // FiniteStreamReader
    template <typename T>
    using finite_stream_reader_type = etl::conjunction<
        is_stream_reader_t<T>,
        is_same<decltype(declval<T>().readable_count()), size_t>,
        is_same<decltype(declval<T>().is_closed()), bool>>;

    template <typename T, typename = void>
    struct is_finite_stream_reader : etl::false_type {};

    template <typename T>
    struct is_finite_stream_reader<T, etl::void_t<finite_stream_reader_type<T>>>
        : finite_stream_reader_type<T> {};

    template <typename T>
    using is_finite_stream_reader_t = typename is_finite_stream_reader<T>::type;
    template <typename T>
    inline constexpr bool is_finite_stream_reader_v = is_finite_stream_reader<T>::value;

    template <typename... Ts>
    using are_all_finite_stream_reader = etl::conjunction<is_finite_stream_reader_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_finite_stream_reader_v =
        are_all_finite_stream_reader<Ts...>::value;

    // StreamWriter
    template <typename T>
    using writer_item = typename etl::remove_reference_t<T>::StreamWriterItem;
    template <typename... Ts>
    using writers_item = etl::enable_if_t<
        etl::are_all_same_v<writer_item<Ts>...>,
        etl::common_type_t<writer_item<Ts>...>>;

    template <typename T>
    using stream_writer_type = etl::conjunction<
        is_same<decltype(declval<T>().write(declval<writer_item<T>>())), bool>,
        is_same<decltype(declval<T>().is_writable()), bool>>;

    template <typename T, typename = void>
    struct is_stream_writer : etl::false_type {};

    template <typename T>
    struct is_stream_writer<T, etl::void_t<stream_writer_type<T>>> : stream_writer_type<T> {};

    template <typename T>
    using is_stream_writer_t = typename is_stream_writer<T>::type;
    template <typename T>
    inline constexpr bool is_stream_writer_v = is_stream_writer<T>::value;

    template <typename... Ts>
    using are_all_stream_writer = etl::conjunction<is_stream_writer_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_stream_writer_v = are_all_stream_writer<Ts...>::value;

    // FiniteStreamWriter
    template <typename T>
    using finite_stream_writer_type = etl::conjunction<
        is_stream_writer_t<T>,
        is_same<decltype(declval<T>().writable_count()), size_t>,
        is_same<decltype(declval<T>().is_closed()), bool>>;

    template <typename T, typename = void>
    struct is_finite_stream_writer : etl::false_type {};

    template <typename T>
    struct is_finite_stream_writer<T, etl::void_t<finite_stream_writer_type<T>>>
        : finite_stream_writer_type<T> {};

    template <typename T>
    using is_finite_stream_writer_t = typename is_finite_stream_writer<T>::type;
    template <typename T>
    inline constexpr bool is_finite_stream_writer_v = is_finite_stream_writer_t<T>::value;

    template <typename... Ts>
    using are_all_finite_stream_writer = etl::conjunction<is_finite_stream_writer_t<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_finite_stream_writer_v =
        are_all_finite_stream_writer<Ts...>::value;

    // FiniteStream
    template <typename T>
    using is_finite_stream =
        etl::disjunction<is_finite_stream_reader_t<T>, is_finite_stream_writer_t<T>>;
    template <typename T>
    inline constexpr bool is_finite_stream_v = is_finite_stream<T>::value;

    template <typename... Ts>
    using are_all_finite_stream = etl::conjunction<is_finite_stream<Ts>...>;
    template <typename... Ts>
    inline constexpr bool are_all_finite_stream_v = are_all_finite_stream<Ts...>::value;
} // namespace nb::stream
