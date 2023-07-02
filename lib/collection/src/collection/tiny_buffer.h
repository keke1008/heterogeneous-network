#pragma once

#include <etl/array.h>
#include <etl/utility.h>
#include <stdint.h>

namespace collection {
    namespace private_tiny_buffer {
        template <typename T, uint8_t N>
        class RawTinyBuffer {
          protected:
            etl::array<T, N> buffer_;

          public:
            inline constexpr RawTinyBuffer() = default;
            inline constexpr RawTinyBuffer(const RawTinyBuffer &) = default;
            inline constexpr RawTinyBuffer(RawTinyBuffer &&) = default;
            inline constexpr RawTinyBuffer &operator=(const RawTinyBuffer &) = default;
            inline constexpr RawTinyBuffer &operator=(RawTinyBuffer &&) = default;

            template <typename... Ts>
            inline constexpr RawTinyBuffer(Ts... ts) : buffer_{ts...} {}

            inline constexpr bool operator==(const RawTinyBuffer &other) const {
                return buffer_ == other.buffer_;
            }

            inline constexpr bool operator!=(const RawTinyBuffer &other) const {
                return buffer_ != other.buffer_;
            }
        };
    } // namespace private_tiny_buffer

    template <typename T, uint8_t N>
    class TinyBuffer : public private_tiny_buffer::RawTinyBuffer<T, N> {
      public:
        using private_tiny_buffer::RawTinyBuffer<T, N>::RawTinyBuffer;

        template <typename... Ts>
        inline constexpr TinyBuffer(const Ts &&...ts)
            : private_tiny_buffer::RawTinyBuffer<T, N>{etl::forward<Ts>(ts)...} {}

        inline constexpr T &operator[](uint8_t index) {
            return this->buffer_[index];
        }

        inline constexpr T &operator[](uint8_t index) const {
            return this->buffer_[index];
        }

        template <uint8_t I>
        inline constexpr T &get() const {
            static_assert(I < N, "Index out of bounds");
            return this->buffer_[I];
        }

        template <uint8_t I>
        inline constexpr void set(T &value) {
            static_assert(I < N, "Index out of bounds");
            this->buffer_[I] = value;
        }

        template <uint8_t I>
        inline constexpr void set(T &&value) {
            static_assert(I < N, "Index out of bounds");
            this->buffer_[I] = etl::move(value);
        }
    };

    template <uint8_t N>
    class TinyBuffer<uint8_t, N> : public private_tiny_buffer::RawTinyBuffer<uint8_t, N> {
      public:
        using private_tiny_buffer::RawTinyBuffer<uint8_t, N>::RawTinyBuffer;

        template <typename... Bytes>
        inline constexpr TinyBuffer(const Bytes... ts)
            : private_tiny_buffer::RawTinyBuffer<uint8_t, N>{static_cast<uint8_t>(ts)...} {}

        inline constexpr uint8_t operator[](uint8_t index) {
            return this->buffer_[index];
        }

        inline constexpr uint8_t operator[](uint8_t index) const {
            return this->buffer_[index];
        }

        template <uint8_t I>
        inline constexpr uint8_t get() const {
            static_assert(I < N, "Index out of bounds");
            return this->buffer_[I];
        }

        template <uint8_t I>
        inline constexpr void set(uint8_t value) {
            static_assert(I < N, "Index out of bounds");
            this->buffer_[I] = value;
        }
    };
} // namespace collection
