#pragma once

#include "./stream.h"
#include <etl/optional.h>
#include <stdint.h>

namespace nb::stream {
    class EmptyStreamReader {
      public:
        EmptyStreamReader() = default;

        inline constexpr bool is_readable() const {
            return false;
        }

        inline constexpr uint8_t readable_count() const {
            return 0;
        }

        inline etl::optional<uint8_t> read() const {
            return etl::nullopt;
        }

        inline constexpr bool is_reader_closed() const {
            return true;
        }
    };

    static_assert(is_stream_reader_v<EmptyStreamReader>);

    class EmptyStreamWriter {
      public:
        EmptyStreamWriter() = default;

        inline constexpr bool is_writable() const {
            return false;
        }

        inline constexpr uint8_t writable_count() const {
            return 0;
        }

        inline constexpr bool write(uint8_t) const {
            return false;
        }

        inline constexpr bool is_writer_closed() const {
            return true;
        }
    };

    static_assert(is_stream_writer_v<EmptyStreamWriter>);

    class DiscardingStreamWriter {
        uint8_t writable_count_;

      public:
        DiscardingStreamWriter() = delete;

        explicit DiscardingStreamWriter(const uint8_t initial_writable_count)
            : writable_count_{initial_writable_count} {}

        inline constexpr bool is_writable() const {
            return writable_count_ > 0;
        }

        inline constexpr uint8_t writable_count() const {
            return writable_count_;
        }

        inline constexpr bool write(const uint8_t) {
            if (writable_count_ > 0) {
                writable_count_--;
                return true;
            }
            return false;
        }

        inline constexpr bool is_writer_closed() const {
            return writable_count_ == 0;
        }
    };

    static_assert(is_stream_writer_v<DiscardingStreamWriter>);

} // namespace nb::stream
