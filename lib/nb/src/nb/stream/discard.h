#pragma once

#include "./types.h"
#include <nb/poll.h>

namespace nb::stream {
    class DiscardingUntilByteWritableBuffer final : public WritableBuffer {
        uint8_t byte_;

      public:
        DiscardingUntilByteWritableBuffer(uint8_t byte) : byte_(byte) {}

        /**
         * readyを返した後に再度呼び出すと，無駄にstreamを消費して死ぬ
         */
        nb::Poll<void> write_all_from(ReadableStream &source) override {
            uint8_t readable_count = source.readable_count();
            for (uint8_t i = 0; i < readable_count; ++i) {
                if (source.read() == byte_) {
                    return nb::ready();
                }
            }
            return nb::pending;
        }
    };
} // namespace nb::stream
