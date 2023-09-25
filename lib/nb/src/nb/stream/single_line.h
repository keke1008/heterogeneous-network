#pragma once

#include <debug_assert.h>
#include <etl/array.h>
#include <etl/span.h>
#include <etl/vector.h>
#include <nb/stream/types.h>

namespace nb::stream {
    namespace {
        inline bool is_complete_line(etl::span<const uint8_t> span) {
            if (span.size() < 2) {
                return false;
            }
            auto last = span.last<2>();
            return last[0] == '\r' && last[1] == '\n';
        }
    } // namespace

    /**
     * `MAX_LENGTH`以下の長さの1行を書き込むバッファ．
     * `MAX_LENGTH`を超える長さの行を書き込んではいけない．
     */
    template <uint8_t MAX_LENGTH>
    class SingleLineWritableBuffer final : public nb::stream::WritableBuffer {
        static_assert(MAX_LENGTH >= 2, "buffer must be able to contain CRLF");

        etl::vector<uint8_t, MAX_LENGTH> buffer_;

      private:
        inline bool is_complete() const {
            return is_complete_line(buffer_);
        }

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            if (is_complete()) {
                return nb::ready();
            }

            uint8_t readable_count = source.readable_count();
            uint8_t writable_count = buffer_.available();
            uint8_t count = etl::min(readable_count, writable_count);
            for (int i = 0; i < count; i++) {
                buffer_.push_back(source.read());
                if (is_complete()) {
                    return nb::ready();
                }
            }

            DEBUG_ASSERT(!buffer_.full(), "line is too long");
            return nb::pending;
        }

        nb::Poll<etl::span<const uint8_t>> poll() const {
            if (is_complete()) {
                return nb::ready(etl::span(buffer_.begin(), buffer_.end()));
            }
            return nb::pending;
        }
    };

    class DiscardSingleLineWritableBuffer final : public nb::stream::WritableBuffer {
        bool is_last_cr_;

      public:
        explicit DiscardSingleLineWritableBuffer() : is_last_cr_{false} {}

        explicit DiscardSingleLineWritableBuffer(uint8_t last_char)
            : is_last_cr_{last_char == '\r'} {}

        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            uint8_t readable_count = source.readable_count();
            for (uint8_t i = 0; i < readable_count; i++) {
                uint8_t byte = source.read();
                if (is_last_cr_ && byte == '\n') {
                    return nb::ready();
                }
                is_last_cr_ = byte == '\r';
            }
            return nb::pending;
        }
    };

    /**
     * 最大`MAX_LENGTH`の長さの1行を書き込むバッファ．
     * `MAX_LENGTH`を超える長さの行は無視される．
     */
    template <uint8_t MAX_LENGTH>
    class MaxLengthSingleLineWrtableBuffer final : public nb::stream::WritableBuffer {
        static_assert(MAX_LENGTH >= 2, "buffer must be able to contain CRLF");

        etl::vector<uint8_t, MAX_LENGTH> buffer_;
        DiscardSingleLineWritableBuffer discard_buffer_;

        bool is_complete() const {
            return is_complete_line(buffer_);
        }

      public:
        nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            if (is_complete()) {
                return nb::ready();
            }

            while (true) {
                if (source.readable_count() == 0) {
                    return nb::pending;
                }

                if (buffer_.full()) {
                    POLL_UNWRAP_OR_RETURN(discard_buffer_.write_all_from(source));
                    buffer_.clear();
                }

                uint8_t readable_count = source.readable_count();
                uint8_t writable_count = buffer_.available();
                uint8_t count = etl::min(readable_count, writable_count);
                for (int i = 0; i < count; i++) {
                    buffer_.push_back(source.read());
                    if (is_complete()) {
                        return nb::ready();
                    }
                }

                if (buffer_.full()) {
                    discard_buffer_ = DiscardSingleLineWritableBuffer(buffer_.back());
                }
            }
        }

        nb::Poll<etl::span<const uint8_t>> poll() const {
            return (is_complete()) ? nb::ready(etl::span(buffer_.begin(), buffer_.end()))
                                   : nb::pending;
        }

        void reset() {
            buffer_.clear();
        }
    };

    /**
     * 少なくとも`MIN_LENGTH`の長さを持つであろう一行を書き込むバッファ．
     * `MIN_LENGTH`バイト書き込まれる中に\r\nが含まれる場合，poll()はfalseを返す．
     * 短すぎる行は自動で捨てられることはない．
     */
    template <uint8_t MIN_LENGTH>
    class MinLengthSingleLineWritableBuffer final : public WritableBuffer {
        etl::vector<uint8_t, MIN_LENGTH> buffer_;

        bool is_complete() const {
            return is_complete_line(buffer_);
        }

      public:
        nb::Poll<void> write_all_from(ReadableStream &source) {
            if (is_complete()) {
                return nb::ready();
            }

            while (true) {
                if (source.readable_count() == 0) {
                    return nb::pending;
                }

                if (buffer_.full()) {
                    return nb::ready();
                }

                uint8_t readable_count = source.readable_count();
                uint8_t writable_count = buffer_.available();
                uint8_t count = etl::min(readable_count, writable_count);
                for (int i = 0; i < count; i++) {
                    buffer_.push_back(source.read());
                    if (is_complete()) {
                        return nb::ready();
                    }
                }
            }
        }

        nb::Poll<bool> poll() const {
            if (is_complete()) {
                return nb::ready(false);
            }
            if (buffer_.full()) {
                return nb::ready(true);
            }
            return nb::pending;
        }

        inline etl::span<const uint8_t> written_bytes() const {
            return etl::span(buffer_.begin(), buffer_.size());
        }
    };
} // namespace nb::stream
