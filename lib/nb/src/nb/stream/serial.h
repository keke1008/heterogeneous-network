#pragma once

#include "./types.h"

namespace nb::stream {
    template <typename RawSerial>
    class SerialStream final : public ReadableWritableStream {
        RawSerial &raw_;

      public:
        SerialStream(RawSerial &raw) : raw_{raw} {}

        SerialStream(const SerialStream &) = delete;
        SerialStream(SerialStream &&) = default;
        SerialStream &operator=(const SerialStream &) = delete;
        SerialStream &operator=(SerialStream &&) = default;

        inline uint8_t readable_count() const override {
            return static_cast<uint8_t>(raw_.available());
        }

        inline uint8_t read() override {
            return static_cast<uint8_t>(raw_.read());
        }

        inline void read(etl::span<uint8_t> buffer) override {
            raw_.readBytes(buffer.data(), buffer.size());
        }

        inline uint8_t writable_count() const override {
            return static_cast<uint8_t>(raw_.availableForWrite());
        }

        inline bool write(uint8_t byte) override {
            raw_.write(byte);
            return readable_count() > 0;
        }

        inline bool write(etl::span<uint8_t> buffer) override {
            raw_.write(buffer.data(), buffer.size());
            return readable_count() > 0;
        }
    };
} // namespace nb::stream
