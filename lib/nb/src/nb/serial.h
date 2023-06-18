#pragma once

#include <etl/optional.h>
#include <stdint.h>

namespace nb {
    template <typename RawSerial>
    class Serial {
        RawSerial &raw_;

      public:
        Serial() = delete;
        Serial(const Serial &) = default;
        Serial(Serial &&) = default;
        Serial &operator=(const Serial &) = default;
        Serial &operator=(Serial &&) = default;

        Serial(RawSerial &raw) : raw_{raw} {}

        inline bool is_ready() const {
            return static_cast<bool>(raw_);
        }

        inline uint8_t readable_bytes() const {
            return raw_.available();
        }

        inline uint8_t writable_bytes() const {
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
            const bool writable = writable_bytes() > 0;
            if (writable) {
                raw_.write(data);
            }
            return writable;
        }
    };
} // namespace nb
