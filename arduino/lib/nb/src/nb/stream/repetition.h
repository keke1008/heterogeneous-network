#pragma once

#include "./types.h"

namespace nb::stream {
    class RepetitionCountWritableBuffer final : public WritableBuffer {
        uint8_t value_;
        uint8_t max_count_;
        uint8_t count_{0};

      public:
        RepetitionCountWritableBuffer(uint8_t value, uint8_t max_count)
            : value_{value},
              max_count_{max_count} {}

        nb::Poll<void> write_all_from(ReadableStream &stream) override {
            if (count_ >= max_count_) {
                return nb::ready();
            }

            uint8_t readable_count = stream.readable_count();
            if (readable_count == 0) {
                return nb::pending;
            }

            for (uint8_t i = 0; i < readable_count; i++) {
                if (stream.read() != value_) {
                    count_ = 0;
                    continue;
                }

                count_++;
                if (count_ >= max_count_) {
                    return nb::ready();
                }
            }
            return nb::pending;
        }
    };

    class RepetitionReadableBuffer final : public ReadableBuffer {
        uint8_t value_;
        uint8_t remaining_count_;

      public:
        RepetitionReadableBuffer(uint8_t value, uint8_t count)
            : value_{value},
              remaining_count_{count} {}

        inline nb::Poll<void> read_all_into(WritableStream &stream) override {
            uint8_t writable_count = stream.writable_count();
            uint8_t write_count = etl::min(writable_count, remaining_count_);
            for (uint8_t i = 0; i < write_count; i++) {
                stream.write(value_);
            }

            remaining_count_ -= write_count;
            return remaining_count_ == 0 ? nb::ready() : nb::pending;
        }
    };
} // namespace nb::stream
