#pragma once

#include <etl/optional.h>
#include <stdint.h>

namespace nb::serial {
    template <typename RawSerial>
    class Serial {
        RawSerial &raw_;

      public:
        using ReadableStreamItem = uint8_t;
        using WritableStreamItem = uint8_t;

        Serial(const Serial &) = delete;
        Serial &operator=(const Serial &) = delete;
        Serial(Serial &&) = default;
        Serial &operator=(Serial &&) = default;

        Serial(RawSerial &raw) : raw_{raw} {}

        inline size_t readable_count() const {
            return raw_.available();
        }

        inline size_t writable_count() const {
            return raw_.availableForWrite();
        }

        inline etl::optional<uint8_t> read() {
            const int value = raw_.read();
            if (value == -1) {
                return etl::nullopt;
            } else {
                return static_cast<uint8_t>(value);
            }
        }

        inline bool write(const uint8_t data) {
            const bool writable = writable_count() > 0;
            if (writable) {
                raw_.write(data);
            }
            return writable;
        }

        bool is_closed() const {
            return static_cast<bool>(raw_);
        }
    };
} // namespace nb::serial
