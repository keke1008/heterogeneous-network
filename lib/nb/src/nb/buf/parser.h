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
    struct AsyncBinParsre {
        T result_;

        template <IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(AsyncBufferSplitter<Buffer> &splitter) {
            auto buffer = POLL_UNWRAP_OR_RETURN(splitter.template split_nbytes<sizeof(T)>());
            result_ = serde::bin::deserialize<T>(buffer).value();
            return nb::ready();
        }

        inline T &result() {
            return result_;
        }
    };

    template <util::unsigned_integral T>
    struct HexParser {
        static constexpr uint8_t SIZE = serde::hex::serialized_size<T>();

        static inline T parse(BufferSplitter &splitter) {
            return serde::hex::deserialize<T>(splitter.split_nbytes<SIZE>()).value();
        }
    };

    template <util::unsigned_integral T>
    struct HexParser {};
} // namespace nb::buf
