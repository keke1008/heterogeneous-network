#pragma once

#include <etl/optional.h>
#include <memory/lifetime.h>
#include <nb/serde.h>
#include <stdint.h>

namespace nb {
    template <typename RawSerial>
    class AsyncReadableWritableSerial {
        RawSerial &raw_;

      public:
        AsyncReadableWritableSerial() = delete;
        AsyncReadableWritableSerial(const AsyncReadableWritableSerial &) = delete;
        AsyncReadableWritableSerial(AsyncReadableWritableSerial &&) = delete;
        AsyncReadableWritableSerial &operator=(const AsyncReadableWritableSerial &) = delete;
        AsyncReadableWritableSerial &operator=(AsyncReadableWritableSerial &&) = delete;

        explicit AsyncReadableWritableSerial(RawSerial &raw) : raw_{raw} {}

        inline nb::Poll<nb::DeserializeResult> poll_readable(uint8_t read_count) {
            return raw_.available() >= read_count ? nb::ready(nb::DeserializeResult::Ok)
                                                  : nb::pending;
        }

        inline uint8_t read_unchecked() {
            return static_cast<uint8_t>(raw_.read());
        }

        inline nb::Poll<nb::DeserializeResult> read(uint8_t &dest) {
            SERDE_DESERIALIZE_OR_RETURN(poll_readable(1));
            dest = read_unchecked();
            return nb::DeserializeResult::Ok;
        }

        inline nb::Poll<nb::SerializeResult> poll_writable(uint8_t write_count) {
            return raw_.availableForWrite() >= write_count ? nb::ready(nb::SerializeResult::Ok)
                                                           : nb::pending;
        }

        inline void write_unchecked(uint8_t data) {
            raw_.write(data);
        }

        inline nb::Poll<nb::SerializeResult> write(uint8_t data) {
            SERDE_SERIALIZE_OR_RETURN(poll_writable(1));
            write_unchecked(data);
            return nb::SerializeResult::Ok;
        }
    };
} // namespace nb
