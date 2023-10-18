#pragma once

#include <debug_assert.h>
#include <etl/span.h>
#include <etl/string_view.h>
#include <util/concepts.h>

namespace nb::buf {
    class BufferBuilder;

    struct [[deprecated("Use IBufferWriter instead")]] BufferWriter {
        virtual void write_to_builder(BufferBuilder &builder) = 0;
    };

    template <typename T>
    concept IBufferWriter = requires(T t, BufferBuilder &builder) {
        { t.write_to_builder(builder) } -> util::same_as<void>;
    };

    class BufferBuilder {
        const etl::span<uint8_t> buffer_;
        uint8_t index_{0};

      public:
        BufferBuilder(etl::span<uint8_t> buffer) : buffer_{buffer} {}

        inline constexpr uint8_t size() const {
            return index_;
        }

        inline constexpr uint8_t writable_count() const {
            return buffer_.size() - index_;
        }

        inline constexpr void append(const uint8_t byte) {
            DEBUG_ASSERT(index_ < buffer_.size());
            buffer_[index_++] = byte;
        }

        inline constexpr void append(const etl::string_view str) {
            uint8_t read_count = str.size();
            DEBUG_ASSERT(index_ + read_count <= buffer_.size());
            etl::copy_n(str.data(), read_count, buffer_.data() + index_);
            index_ += read_count;
        }

        inline constexpr void append(const etl::span<const uint8_t> bytes) {
            uint8_t read_count = bytes.size();
            DEBUG_ASSERT(index_ + read_count <= buffer_.size());
            etl::copy_n(bytes.data(), read_count, buffer_.data() + index_);
            index_ += read_count;
        }

        /**
         * F: etl::span<uint8_t> -> uint8_t
         */
        template <
            typename F,
            etl::enable_if_t<
                etl::is_same_v<
                    decltype(etl::declval<F>()(etl::declval<etl::span<uint8_t>>())),
                    uint8_t>,
                uint8_t> = 0>
        inline constexpr void append(F &&f) {
            auto span = etl::span(buffer_.data() + index_, buffer_.end());
            index_ += f(span);
            DEBUG_ASSERT(index_ <= buffer_.size());
        }

        [[deprecated("Use append(IBufferWriter&&) instead")]] inline void
        append(BufferWriter &writer) {
            writer.write_to_builder(*this);
        }

        [[deprecated("Use append(IBufferWriter&&) instead")]] inline void
        append(BufferWriter &&writer) {
            writer.write_to_builder(*this);
        }

        template <IBufferWriter T>
        inline void append(T &&writer) {
            writer.write_to_builder(*this);
        }

        template <IBufferWriter T>
        inline void append(const T &writer) {
            writer.write_to_builder(*this);
        }
    };

    template <typename... Args>
    etl::span<uint8_t> build_buffer(etl::span<uint8_t> buffer, Args &&...args) {
        BufferBuilder builder{buffer};
        (builder.append(etl::forward<Args>(args)), ...);
        return etl::span(buffer.data(), builder.size());
    }
} // namespace nb::buf
