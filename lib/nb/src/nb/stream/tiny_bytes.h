#pragma once

#include "./stream.h"
#include <collection/tiny_buffer.h>
#include <etl/array.h>
#include <etl/optional.h>
#include <nb/poll.h>
#include <stdint.h>

namespace nb::stream {
    template <uint8_t N>
    class TinyByteReader {
        uint8_t read_bytes_{0};
        collection::TinyBuffer<uint8_t, N> buffer_;

      public:
        constexpr TinyByteReader() = default;
        constexpr TinyByteReader(const TinyByteReader &) = default;
        constexpr TinyByteReader(TinyByteReader &&) = default;
        constexpr TinyByteReader &operator=(const TinyByteReader &) = default;
        constexpr TinyByteReader &operator=(TinyByteReader &&) = default;

        inline constexpr TinyByteReader(const collection::TinyBuffer<uint8_t, N> &buffer)
            : buffer_{buffer} {}

        template <typename... Bytes>
        inline constexpr TinyByteReader(const Bytes... bytes) : buffer_{bytes...} {
            static_assert(sizeof...(Bytes) == N, "Wrong number of arguments");
        }

        inline constexpr bool is_readable() const {
            return read_bytes_ < N;
        }

        inline constexpr uint8_t readable_count() const {
            return N - read_bytes_;
        }

        inline etl::optional<uint8_t> read() {
            if (is_readable()) {
                return buffer_[read_bytes_++];
            }
            return etl::nullopt;
        }

        inline bool is_reader_closed() const {
            return read_bytes_ >= N;
        }
    };

    static_assert(is_stream_reader_v<TinyByteReader<1>>);

    template <uint8_t N>
    class TinyByteWriter {
        uint8_t written_bytes_{0};
        collection::TinyBuffer<uint8_t, N> bytes_;

      public:
        constexpr TinyByteWriter() = default;
        constexpr TinyByteWriter(const TinyByteWriter &) = default;
        constexpr TinyByteWriter(TinyByteWriter &&) = default;
        constexpr TinyByteWriter &operator=(const TinyByteWriter &) = default;
        constexpr TinyByteWriter &operator=(TinyByteWriter &&) = default;

        constexpr inline bool is_writable() const {
            return N > written_bytes_;
        }

        constexpr inline uint8_t writable_count() const {
            return N - written_bytes_;
        }

        constexpr inline bool write(uint8_t data) {
            if (is_writable()) {
                bytes_[written_bytes_++] = data;
                return true;
            }
            return false;
        }

        constexpr inline bool is_writer_closed() const {
            return written_bytes_ >= N;
        }

        nb::Poll<const collection::TinyBuffer<uint8_t, N> &&> poll() const {
            if (is_writer_closed()) {
                return etl::move(bytes_);
            }
            return nb::pending;
        }
    };

    static_assert(is_stream_writer_v<TinyByteWriter<1>>);
} // namespace nb::stream
