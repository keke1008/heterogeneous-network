#pragma once

#include "stream.h"
#include <etl/array.h>
#include <etl/optional.h>

namespace nb::stream {
    template <size_t N>
    class FixedByteReader {

        size_t read_bytes_ = 0;
        etl::array<uint8_t, N> bytes_;

      public:
        using ReadableStreamItem = uint8_t;

        FixedByteReader(const etl::array<uint8_t, N> &bytes) : bytes_{bytes} {}

        FixedByteReader(etl::array<uint8_t, N> &&bytes) : bytes_{bytes} {}

        inline size_t readable_count() const {
            return N - read_bytes_;
        }

        inline bool is_closed() const {
            return read_bytes_ >= N;
        }

        inline etl::optional<uint8_t> read() {
            return is_closed() ? etl::nullopt : etl::make_optional(bytes_[read_bytes_++]);
        }
    };

    template <typename... Ts>
    static FixedByteReader<sizeof...(Ts)> make_fixed_stream_reader(Ts &&...ts) {
        return FixedByteReader<sizeof...(Ts)>{etl::make_array<uint8_t>(etl::forward<Ts>(ts)...)};
    }

    static_assert(is_stream_reader_v<FixedByteReader<0>, uint8_t>);

    template <size_t N>
    class FixedByteWriter {
        size_t written_bytes = 0;
        etl::array<uint8_t, N> bytes_;

      public:
        using WritableStreamItem = uint8_t;

        FixedByteWriter() : bytes_{} {}

        inline size_t writable_count() const {
            return N - written_bytes;
        }

        inline bool is_closed() const {
            return written_bytes >= N;
        }

        inline bool write(uint8_t data) {
            if (is_closed()) {
                return false;
            } else {
                bytes_[written_bytes++] = data;
                return true;
            }
        }

        inline etl::array<uint8_t, N> &get() {
            return bytes_;
        }
    };

    static_assert(is_stream_writer_v<FixedByteWriter<0>, uint8_t>);
} // namespace nb::stream
