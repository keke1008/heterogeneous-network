#pragma once

#include "./splitter.h"
#include <serde/bin.h>
#include <serde/hex.h>
#include <util/concepts.h>

namespace nb::buf {
    template <util::unsigned_integral T>
    struct BinParser {
        static inline T parse(BufferSplitter &splitter) {
            return serde::bin::deserialize<T>(splitter.split_nbytes<sizeof(T)>()).value();
        }
    };

    template <util::unsigned_integral T>
    struct HexParser {
        static constexpr uint8_t SIZE = serde::hex::serialized_size<T>();

        static inline T parse(BufferSplitter &splitter) {
            return serde::hex::deserialize<T>(splitter.split_nbytes<SIZE>()).value();
        }
    };
} // namespace nb::buf
