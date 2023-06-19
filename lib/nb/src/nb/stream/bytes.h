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

        FixedByteReader(etl::array<uint8_t, N> bytes) : bytes_{bytes} {}

        inline size_t readable_count() const {
            return N - read_bytes_;
        }

        inline etl::optional<uint8_t> read() {
            return is_closed() ? etl::nullopt : etl::make_optional(bytes_[read_bytes_++]);
        }

        inline bool is_closed() const {
            return read_bytes_ >= N;
        }
    };

    static_assert(is_stream_reader_v<FixedByteReader<0>, uint8_t>);
} // namespace nb::stream
