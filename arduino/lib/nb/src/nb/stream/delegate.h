#pragma once

#include "./empty.h"
#include "./stream.h"
#include <etl/optional.h>
#include <etl/utility.h>

namespace nb::stream {
    template <typename R>
    class StreamReaderDelegate {
        static_assert(is_stream_reader_v<
                      etl::decay_t<decltype(etl::declval<R>().delegate_reader())>>);
        R reader_;

      public:
        StreamReaderDelegate() = default;

        template <typename... Ts>
        inline StreamReaderDelegate(Ts &&...ts) : reader_{etl::forward<Ts>(ts)...} {}

        inline constexpr bool is_readable() const {
            return reader_.delegate_reader().is_readable();
        }

        inline constexpr decltype(auto) readable_count() const {
            return reader_.delegate_reader().readable_count();
        }

        inline constexpr etl::optional<uint8_t> read() {
            return reader_.delegate_reader().read();
        }

        inline constexpr bool is_reader_closed() const {
            return reader_.delegate_reader().is_reader_closed();
        }
    };

    template <typename W>
    class StreamWriterDelegate {
        static_assert(is_stream_writer_v<
                      etl::decay_t<decltype(etl::declval<W>().delegate_writer())>>);
        W writer_;

      public:
        StreamWriterDelegate() = default;

        template <typename... Ts>
        inline StreamWriterDelegate(Ts &&...ts) : writer_{etl::forward<Ts>(ts)...} {}

        inline constexpr bool is_writable() const {
            return writer_.delegate_writer().is_writable();
        }

        inline constexpr decltype(auto) writable_count() const {
            return writer_.delegate_writer().writable_count();
        }

        inline constexpr bool write(uint8_t byte) {
            return writer_.delegate_writer().write(byte);
        }

        inline constexpr bool is_writer_closed() const {
            return writer_.delegate_writer().is_writer_closed();
        }

        inline constexpr W &get_writer() {
            return writer_;
        }

        inline constexpr const W &get_writer() const {
            return writer_;
        }
    };

    template <typename RW>
    class StreamReaderWriterDelegate {
        static_assert(is_stream_reader_v<
                      etl::decay_t<decltype(etl::declval<RW>().delegate_reader())>>);
        static_assert(is_stream_writer_v<
                      etl::decay_t<decltype(etl::declval<RW>().delegate_writer())>>);
        RW reader_writer_;

      public:
        StreamReaderWriterDelegate() = default;

        template <typename... Ts>
        inline StreamReaderWriterDelegate(Ts &&...ts) : reader_writer_{etl::forward<Ts>(ts)...} {}

        inline constexpr bool is_readable() const {
            return reader_writer_.delegate_reader().is_readable();
        }

        inline constexpr decltype(auto) readable_count() const {
            return reader_writer_.delegate_reader().readable_count();
        }

        inline constexpr etl::optional<uint8_t> read() {
            return reader_writer_.delegate_reader().read();
        }

        inline constexpr bool is_reader_closed() const {
            return reader_writer_.delegate_reader().is_reader_closed();
        }

        inline constexpr bool is_writable() const {
            return reader_writer_.delegate_writer().is_writable();
        }

        inline constexpr decltype(auto) writable_count() const {
            return reader_writer_.delegate_writer().writable_count();
        }

        inline constexpr bool write(uint8_t byte) {
            return reader_writer_.delegate_writer().write(byte);
        }

        inline constexpr bool is_writer_closed() const {
            return reader_writer_.delegate_writer().is_writer_closed();
        }

        inline constexpr RW &get_reader_writer() {
            return reader_writer_;
        }
    };

    template <typename Derived>
    class DelegateStreamReader {
      public:
        inline constexpr bool is_readable() const {
            return static_cast<const Derived *>(this)->delegate_reader().is_readable();
        }

        inline constexpr decltype(auto) readable_count() const {
            return static_cast<const Derived *>(this)->delegate_reader().readable_count();
        }

        inline constexpr etl::optional<uint8_t> read() {
            return static_cast<const Derived *>(this)->delegate_reader().read();
        }

        inline constexpr bool is_reader_closed() const {
            return static_cast<const Derived *>(this)->delegate_reader().is_reader_closed();
        }
    };

    template <typename Derived>
    class DelegateStreamWriter {
      public:
        inline constexpr bool is_writable() const {
            return static_cast<const Derived *>(this)->delegate_writer().is_writable();
        }

        inline constexpr decltype(auto) writable_count() const {
            return static_cast<const Derived *>(this)->delegate_writer().writable_count();
        }

        inline constexpr bool write(uint8_t byte) {
            return static_cast<const Derived *>(this)->delegate_writer().write(byte);
        }

        inline constexpr bool is_writer_closed() const {
            return static_cast<const Derived *>(this)->delegate_writer().is_writer_closed();
        }
    };
} // namespace nb::stream
