#pragma once

#include "./types.h"
#include <etl/vector.h>
#include <stdint.h>

namespace nb::stream {
    /**
     * 番兵のバイトが来るまでバッファに書き込み続ける
     * 読み込まれるストリームは，ちょうど番兵のバイトを含まない状態になる
     */
    template <uint8_t MAX_BUFFER_SIZE>
    class SentinelWritableBuffer : public WritableBuffer {
        etl::vector<uint8_t, MAX_BUFFER_SIZE> buffer_;
        uint8_t sentinel_;

      public:
        explicit SentinelWritableBuffer(uint8_t sentinel) : sentinel_{sentinel} {}

        nb::Poll<void> write_all_from(ReadableStream &stream) override {
            uint8_t count = etl::min(stream.readable_count(), buffer_.available());
            if (count == 0) {
                return nb::pending;
            }

            for (uint8_t i = 0; i < count; ++i) {
                uint8_t byte = stream.read();
                if (byte == sentinel_) {
                    return nb::ready();
                }
                buffer_.push_back(byte);
            }

            return nb::pending;
        }

        etl::span<const uint8_t> written_bytes() const {
            return etl::span<const uint8_t>{buffer_.data(), buffer_.size()};
        }
    };
} // namespace nb::stream
