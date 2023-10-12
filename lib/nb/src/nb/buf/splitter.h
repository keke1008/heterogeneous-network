#pragma once

#include <debug_assert.h>
#include <etl/span.h>
#include <util/concepts.h>

namespace nb::buf {
    class BufferSplitter;

    template <typename T>
    struct [[deprecated("use IBufferParser instead")]] BufferParser {
        virtual T parse(BufferSplitter &splitter) = 0;
    };

    template <typename T>
    concept IBufferParser = requires(T &parser, BufferSplitter &splitter) {
        { parser.parse(splitter) };
    };

    class BufferSplitter {
        etl::span<const uint8_t> buffer_;
        uint8_t index_{0};

      public:
        BufferSplitter(etl::span<const uint8_t> buffer) : buffer_{buffer} {}

        inline bool is_empty() const {
            return index_ == buffer_.size();
        }

        /**
         * 1byte読み進める
         */
        inline constexpr uint8_t split_1byte() {
            DEBUG_ASSERT(index_ < buffer_.size());
            return buffer_[index_++];
        }

        template <uint8_t N>
        inline constexpr etl::span<const uint8_t, N> split_nbytes() {
            DEBUG_ASSERT(index_ + N <= buffer_.size());
            etl::span<const uint8_t, N> span{buffer_.data() + index_, N};
            index_ += N;
            return span;
        }

        inline constexpr etl::span<const uint8_t> split_nbytes(uint8_t n) {
            DEBUG_ASSERT(index_ + n <= buffer_.size());
            auto span = buffer_.subspan(index_, n);
            index_ += n;
            return span;
        }

        /**
         * `sentinel`が現れるまで読み進める
         * 戻り値の範囲は，`sentinel`を含まない
         * 次のparseは`sentinel`の次のバイトから始まる
         */
        inline constexpr etl::span<const uint8_t> split_sentinel(uint8_t sentinel) {
            auto begin = buffer_.data() + index_;
            while (index_ < buffer_.size()) {
                if (split_1byte() == sentinel) {
                    return etl::span<const uint8_t>{begin, buffer_.data() + index_ - 1};
                }
            }
            DEBUG_ASSERT(false, "sentinel not found");
        }

        /**
         * F: uint8_t -> bool
         */
        template <typename F>
        inline constexpr etl::span<const uint8_t> split_while(F &&f) {
            uint8_t start = index_;
            while (index_ < buffer_.size()) {
                if (!f(buffer_[index_])) {
                    return etl::span{buffer_.data() + start, buffer_.data() + index_};
                }
                index_++;
            }
            DEBUG_ASSERT(false, "sentinel not found");
        }

        inline constexpr etl::span<const uint8_t> split_remaining() {
            auto span = buffer_.subspan(index_);
            index_ = buffer_.size();
            return span;
        }

        /**
         * F: etl::span<const uint8_t> -> uint8_t
         */
        template <typename F>
        inline constexpr void split(F &&f) {
            auto span = buffer_.subspan(index_);
            index_ += f(span);
        }

        template <typename T>
        inline constexpr T parse(BufferParser<T> &parser) {
            return parser.parse(*this);
        }

        template <typename T>
        [[deprecated("use parse<IBufferParser>() instead")]] inline constexpr T
        parse(BufferParser<T> &&parser) {
            return parser.parse(*this);
        }

        template <typename Parser>
        [[deprecated("use parse<IBufferParser>() instead")]] inline constexpr decltype(auto)
        parse() {
            return Parser{}.parse(*this);
        }

        template <IBufferParser Parser>
        inline constexpr decltype(auto) parse() {
            return Parser{}.parse(*this);
        }

        template <IBufferParser Parser>
        inline constexpr decltype(auto) parse(Parser &&parser) {
            return parser.parse(*this);
        }
    };

    template <IBufferParser Parser>
    inline decltype(auto) parse(etl::span<const uint8_t> buffer) {
        BufferSplitter splitter{buffer};
        return Parser{}.parse(splitter);
    }

    template <IBufferParser Parser>
    inline decltype(auto) parse(etl::span<const uint8_t> buffer, Parser &&parser) {
        BufferSplitter splitter{buffer};
        return parser.parse(splitter);
    }
} // namespace nb::buf
