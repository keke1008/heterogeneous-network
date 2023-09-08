#pragma once

#include "./types.h"
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/functional.h>
#include <etl/initializer_list.h>
#include <etl/span.h>

namespace nb::stream {
    namespace _private_readable_buffer {
        inline void initialize_array_single(uint8_t **ptr, uint8_t value) {
            **ptr = value;
            ++*ptr;
        }

        inline void initialize_array_single(uint8_t **ptr, etl::span<uint8_t> values) {
            for (auto value : values) {
                **ptr = value;
                ++*ptr;
            }
        }

        inline void initialize_array_single(uint8_t **ptr, ReadableStream &reader) {
            uint8_t *end = *ptr + reader.readable_count();
            reader.read(etl::span(*ptr, end));
            *ptr = end;
        }

        template <uint8_t N, typename... Rs>
        uint8_t initialize_array(etl::array<uint8_t, N> &array, Rs &&...rs) {
            uint8_t *end = array.data();
            (initialize_array_single(&end, etl::forward<Rs>(rs)), ...);
            return end - array.data();
        }
    } // namespace _private_readable_buffer

    template <uint8_t MAX_LENGTH>
    class FixedReadableBuffer final : public ReadableStream, public ReadableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        uint8_t index_{0};

      public:
        FixedReadableBuffer() = delete;

        template <typename... Rs>
        FixedReadableBuffer(Rs &&...rs) {
            length_ = _private_readable_buffer::initialize_array<MAX_LENGTH, Rs...>(
                bytes_, etl::forward<Rs>(rs)...
            );
        }

        inline uint8_t readable_count() const override {
            return length_ - index_;
        }

        inline uint8_t read() override {
            return bytes_[index_++];
        }

        void read(etl::span<uint8_t> buffer) override {
            uint8_t read_count = etl::min(readable_count(), static_cast<uint8_t>(buffer.size()));
            etl::copy_n(bytes_.data() + index_, read_count, buffer.begin());
            index_ += read_count;
        }

        nb::Poll<void> write_to(WritableStream &destination) override {
            bool continue_ = true;
            while (continue_) {
                uint8_t write_count = etl::min(readable_count(), destination.writable_count());
                if (write_count == 0) {
                    break;
                }
                continue_ = destination.write(etl::span(bytes_.data() + index_, write_count));
                index_ += write_count;
            }
            return index_ < length_ ? nb::pending : nb::ready();
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedWritableBuffer final : public WritableStream, public WritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        uint8_t index_{0};

        inline bool is_writable() const {
            return index_ < length_;
        }

      public:
        FixedWritableBuffer() : length_{MAX_LENGTH} {};
        FixedWritableBuffer(uint8_t length) : length_{length} {};

        inline uint8_t writable_count() const override {
            return length_ - index_;
        }

        inline bool write(uint8_t byte) override {
            if (is_writable()) {
                bytes_[index_++] = byte;
            }
            return is_writable();
        }

        bool write(etl::span<uint8_t> buffer) override {
            uint8_t write_count = etl::min(writable_count(), static_cast<uint8_t>(buffer.size()));
            etl::copy_n(buffer.begin(), write_count, bytes_.data() + index_);
            index_ += write_count;
            return is_writable();
        }

        nb::Poll<void> read_from(ReadableStream &source) override {
            uint8_t read_count = etl::min(writable_count(), source.readable_count());
            source.read(etl::span(bytes_.data() + index_, read_count));
            index_ += read_count;
            return is_writable() ? nb::pending : nb::ready();
        }

      public:
        nb::Poll<etl::span<uint8_t>> poll() {
            return is_writable() ? nb::pending : nb::ready(etl::span(bytes_.data(), index_));
        }
    };
} // namespace nb::stream
