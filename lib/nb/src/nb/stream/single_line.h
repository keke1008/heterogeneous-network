#pragma once

#include <etl/array.h>
#include <etl/span.h>
#include <nb/stream/types.h>

namespace nb::stream {
    template <uint8_t MAX_LENGTH>
    class SingleLineWritableBuffer final : public nb::stream::WritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> buffer_;
        uint8_t index_ = 0;

      private:
        inline bool is_complete() const {
            if (index_ < 2) {
                return false;
            }
            return buffer_[index_ - 2] == '\r' && buffer_[index_ - 1] == '\n';
        }

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            if (is_complete()) {
                return nb::ready();
            }

            while (true) {
                uint8_t writable_count = (index_ >= 1 && buffer_[index_ - 1] == '\r') ? 1 : 2;
                uint8_t read_count = etl::min(source.readable_count(), writable_count);
                if (read_count == 0) {
                    return nb::pending;
                }

                source.read(etl::span(buffer_.data() + index_, read_count));
                index_ += read_count;
                if (is_complete()) {
                    return nb::ready();
                }
            }
        }

        nb::Poll<etl::span<const uint8_t>> poll() const {
            if (is_complete()) {
                return nb::ready(etl::span(buffer_.data(), index_));
            }
            return nb::pending;
        }
    };
} // namespace nb::stream
