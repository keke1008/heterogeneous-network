#pragma once

#include <etl/optional.h>
#include <mock/serial.h>
#include <nb/stream.h>
#include <stdint.h>

namespace nb::serial {
    template <typename RawSerial>
    class Serial {
        RawSerial &raw_;

      public:
        using StreamReaderItem = uint8_t;
        using StreamWriterItem = uint8_t;

        Serial(const Serial &) = delete;
        Serial &operator=(const Serial &) = delete;
        Serial(Serial &&) = default;
        Serial &operator=(Serial &&) = default;

        Serial(RawSerial &raw) : raw_{raw} {}

        inline etl::optional<uint8_t> read() {
            const int value = raw_.read();
            if (value == -1) {
                return etl::nullopt;
            } else {
                return static_cast<uint8_t>(value);
            }
        }

        inline bool write(const uint8_t data) {
            const bool writable = raw_.availableForWrite() > 0;
            if (writable) {
                raw_.write(data);
            }
            return writable;
        }

        inline size_t readable_count() const {
            return raw_.available();
        }

        inline size_t writable_count() const {
            return raw_.availableForWrite();
        }

        inline bool is_readable() const {
            return readable_count() > 0;
        }

        inline bool is_writable() const {
            return writable_count() > 0;
        }

        inline bool is_reader_closed() const {
            return !static_cast<bool>(raw_);
        }

        inline bool is_writer_closed() const {
            return !static_cast<bool>(raw_);
        }
    };

    static_assert(nb::stream::is_stream_reader_v<Serial<mock::MockSerial>>);
    static_assert(nb::stream::is_stream_writer_v<Serial<mock::MockSerial>>);
} // namespace nb::serial
