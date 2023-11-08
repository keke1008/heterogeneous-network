#pragma once

#include "./splitter.h"
#include <etl/optional.h>
#include <serde/bin.h>
#include <serde/hex.h>
#include <util/concepts.h>

namespace nb::buf {
    template <util::unsigned_integral T>
    struct BinParser {
        static inline T parse(BufferSplitter &splitter) {
            return serde::bin::deserialize<T>(splitter.split_nbytes<sizeof(T)>());
        }
    };

    template <util::unsigned_integral T>
    struct AsyncBinParsre {
        static constexpr uint8_t SIZE = sizeof(T);

        etl::optional<T> result_;

        template <IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(AsyncBufferSplitter<Buffer> &splitter) {
            if (result_.has_value()) {
                return nb::ready();
            }
            auto buffer = POLL_UNWRAP_OR_RETURN(splitter.template split_nbytes<SIZE>());
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
    struct AsyncHexParser {
        static constexpr uint8_t SIZE = serde::hex::serialized_size<T>();

        etl::optional<T> result_;

        template <IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(AsyncBufferSplitter<Buffer> &splitter) {
            if (result_.has_value()) {
                return nb::ready();
            }
            auto buffer = POLL_UNWRAP_OR_RETURN(splitter.template split_nbytes<SIZE>());
            result_ = serde::hex::deserialize<T>(buffer).value();
            return nb::ready();
        }
    };

    template <typename Enum>
    struct AsyncOneByteEnumParser {
        static_assert(etl::is_enum_v<Enum>, "Enum must be an enum type");
        static_assert(sizeof(Enum) == 1, "Enum must be 1 byte");

        etl::optional<Enum> result_;

        template <IAsyncBuffer Buffer>
        inline nb::Poll<void> parse(AsyncBufferSplitter<Buffer> &splitter) {
            if (result_.has_value()) {
                return nb::ready();
            }
            auto buffer = POLL_UNWRAP_OR_RETURN(splitter.template split_1byte());
            result_ = static_cast<Enum>(buffer);
            return nb::ready();
        }

        inline Enum result() {
            return result_;
        }
    };
} // namespace nb::buf
