#pragma once

#include "./types.h"
#include <nb/poll.h>

namespace nb::stream {
    class DiscardingUntilByteWritableBuffer final : public WritableBuffer {
        uint8_t byte_;
        bool done_{false};

      public:
        DiscardingUntilByteWritableBuffer(uint8_t byte) : byte_(byte) {}

        nb::Poll<void> write_all_from(ReadableStream &source) override {
            if (done_) {
                return nb::ready();
            }

            uint8_t readable_count = source.readable_count();
            for (uint8_t i = 0; i < readable_count; ++i) {
                if (source.read() == byte_) {
                    done_ = true;
                    return nb::ready();
                }
            }
            return nb::pending;
        }
    };

    class DiscardingCountWritableBuffer final : public WritableBuffer {
        uint8_t remaining_count_;

      public:
        explicit DiscardingCountWritableBuffer(uint8_t count) : remaining_count_{count} {}

        nb::Poll<void> write_all_from(ReadableStream &source) override {
            uint8_t write_count = etl::min(source.readable_count(), remaining_count_);
            for (uint8_t i = 0; i < write_count; ++i) {
                source.read();
            }
            remaining_count_ -= write_count;
            return remaining_count_ == 0 ? nb::ready() : nb::pending;
        }
    };
} // namespace nb::stream