#pragma once

#include <debug_assert.h>
#include <etl/span.h>
#include <nb/poll.h>
#include <util/concepts.h>

namespace nb::buf {
    class BufferSplitter;

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

        inline uint8_t splitted_count() const {
            return index_;
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

    template <typename T>
    concept IAsyncBuffer = requires(T &buffer) {
        // 読み込み可能なバッファ
        { buffer.span() } -> util::same_as<etl::span<const uint8_t>>;
    };

    template <IAsyncBuffer Buffer>
    class AsyncBufferSplitter;

    template <typename AsyncParser, typename Buffer>
    concept IAsyncParser = IAsyncBuffer<Buffer> &&
        requires(AsyncParser &parser, AsyncBufferSplitter<Buffer> &splitter) {
            { parser.parse(splitter) } -> util::same_as<nb::Poll<void>>;
            { parser.result() };
        };

    template <IAsyncBuffer Buffer>
    class AsyncBufferSplitter {
        Buffer &buffer_;
        uint8_t index_{0};

      public:
        explicit AsyncBufferSplitter(Buffer &buffer) : buffer_{buffer} {}

        inline uint8_t splitted_count() const {
            return index_;
        }

        inline nb::Poll<uint8_t> split_1byte() {
            auto span = buffer_.span();
            if (index_ < span.size()) {
                return buffer_.span()[index_++];
            }
            return nb::pending;
        }

        template <uint8_t N>
        inline nb::Poll<etl::span<const uint8_t, N>> split_nbytes() {
            auto span = buffer_.span();
            if (index_ + N <= span.size()) {
                etl::span<const uint8_t, N> span{span().data() + index_, N};
                index_ += N;
                return etl::move(span);
            }
            return nb::pending;
        }

        inline nb::Poll<etl::span<const uint8_t>> split_nbytes(uint8_t n) {
            auto span = buffer_.span();
            if (index_ + n <= span.size()) {
                auto sub_span = span.subspan(index_, n);
                index_ += n;
                return etl::move(sub_span);
            }
            return nb::pending;
        }

        /**
         * `sentinel`が現れるまで読み進める
         * 戻り値の範囲は`sentinel`を含まない
         * 次のparseは`sentinel`の次のバイトから始まる
         */
        inline nb::Poll<etl::span<const uint8_t>> split_sentinel(uint8_t sentinel) {
            const auto span = buffer_.span();
            uint8_t begin = span.data() + index_;
            uint8_t index = index_;
            while (index < span.size()) {
                if (span[index++] == sentinel) {
                    index_ = index;
                    return etl::span<const uint8_t>{begin, span.data() + index - 1};
                }
            }

            return nb::pending;
        }

        inline constexpr etl::span<const uint8_t> split_remaining() {
            auto span = buffer_.span().subspan(index_);
            index_ = span.size();
            return span;
        }

        template <IAsyncParser<Buffer> Parser>
        inline nb::Poll<void> parse(Parser &parser) {
            return parser.parse(*this);
        }
    };
} // namespace nb::buf
