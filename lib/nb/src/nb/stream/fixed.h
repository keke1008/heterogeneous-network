#pragma once

#include "./types.h"
#include <etl/algorithm.h>
#include <etl/array.h>
#include <etl/functional.h>
#include <etl/initializer_list.h>
#include <etl/span.h>

namespace nb::stream {
    namespace _private_fixed {
        inline void initialize_array_single(uint8_t **ptr, uint8_t value) {
            **ptr = value;
            ++*ptr;
        }

        inline void initialize_array_single(uint8_t **ptr, etl::span<const uint8_t> values) {
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

        template <uint8_t MAX_LENGTH>
        class FixedReadableBufferIndex {
            uint8_t read_index_{0};

          public:
            inline uint8_t readable_count(uint8_t readable_length) const {
                return readable_length - read_index_;
            }

            inline uint8_t
            read(const etl::array<uint8_t, MAX_LENGTH> &bytes, uint8_t readable_length) {
                return bytes[read_index_++];
            }

            void read(
                const etl::array<uint8_t, MAX_LENGTH> &bytes,
                uint8_t readable_length,
                etl::span<uint8_t> buffer
            ) {
                uint8_t buffer_size = static_cast<uint8_t>(buffer.size());
                uint8_t read_count = etl::min(readable_count(readable_length), buffer_size);
                etl::copy_n(bytes.data() + read_index_, read_count, buffer.begin());
                read_index_ += read_count;
            }

            nb::Poll<void> read_all_into(
                const etl::array<uint8_t, MAX_LENGTH> &bytes,
                uint8_t readable_length,
                WritableStream &destination
            ) {
                bool continue_ = true;
                while (continue_) {
                    uint8_t readable_count_ = readable_count(readable_length);
                    uint8_t writable_count = destination.writable_count();
                    uint8_t write_count = etl::min(readable_count_, writable_count);
                    if (write_count == 0) {
                        break;
                    }

                    auto span = etl::span(bytes.data() + read_index_, write_count);
                    continue_ = destination.write(span);
                    read_index_ += write_count;
                }
                return read_index_ < readable_length ? nb::pending : nb::ready();
            }
        };

        template <uint8_t MAX_LENGTH>
        class FixedWritableBufferIndex {
            uint8_t index_{0};

          public:
            inline uint8_t index() const {
                return index_;
            }

            inline bool is_writable(uint8_t writable_length) const {
                return index_ < writable_length;
            }

            inline uint8_t writable_count(uint8_t writable_length) const {
                return writable_length - index_;
            }

            inline bool
            write(etl::array<uint8_t, MAX_LENGTH> &bytes, uint8_t writable_length, uint8_t byte) {
                bytes[index_++] = byte;
                return is_writable(writable_length);
            }

            bool write(
                etl::array<uint8_t, MAX_LENGTH> &bytes,
                uint8_t writable_length,
                etl::span<const uint8_t> buffer
            ) {
                uint8_t buffer_size = static_cast<uint8_t>(buffer.size());
                uint8_t write_count = etl::min(writable_count(writable_length), buffer_size);
                etl::copy_n(buffer.begin(), write_count, bytes.data() + index_);
                index_ += write_count;
                return is_writable(writable_length);
            }

            nb::Poll<void> write_all_from(
                etl::array<uint8_t, MAX_LENGTH> &bytes,
                uint8_t writable_length,
                ReadableStream &source
            ) {
                uint8_t read_count =
                    etl::min(writable_count(writable_length), source.readable_count());
                source.read(etl::span(bytes.data() + index_, read_count));
                index_ += read_count;
                return is_writable(writable_length) ? nb::pending : nb::ready();
            }
        };

    } // namespace _private_fixed

    template <uint8_t MAX_LENGTH>
    class FixedReadableBuffer final : public ReadableStream, public ReadableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        _private_fixed::FixedReadableBufferIndex<MAX_LENGTH> index_;

      public:
        FixedReadableBuffer() = delete;

        template <typename... Rs>
        FixedReadableBuffer(Rs &&...rs) {
            length_ = _private_fixed::initialize_array<MAX_LENGTH, Rs...>(
                bytes_, etl::forward<Rs>(rs)...
            );
        }

        inline uint8_t readable_count() const override {
            return index_.readable_count(length_);
        }

        inline uint8_t read() override {
            return index_.read(bytes_, length_);
        }

        void read(etl::span<uint8_t> buffer) override {
            index_.read(bytes_, length_, buffer);
        }

        nb::Poll<void> read_all_into(WritableStream &destination) override {
            return index_.read_all_into(bytes_, length_, destination);
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedWritableBuffer final : public WritableStream, public WritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        _private_fixed::FixedWritableBufferIndex<MAX_LENGTH> index_;

      public:
        FixedWritableBuffer() : length_{MAX_LENGTH} {};

        FixedWritableBuffer(uint8_t length) : length_{length} {};

        inline uint8_t writable_count() const override {
            return index_.writable_count(length_);
        }

        inline bool write(uint8_t byte) override {
            return index_.write(bytes_, length_, byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return index_.write(bytes_, length_, buffer);
        }

        inline nb::Poll<void> write_all_from(ReadableStream &source) override {
            return index_.write_all_from(bytes_, length_, source);
        }

      public:
        nb::Poll<etl::span<uint8_t>> poll() {
            return index_.is_writable(length_)
                       ? nb::pending
                       : nb::ready(etl::span(bytes_.data(), index_.index()));
        }
    };

    template <uint8_t MAX_LENGTH>
    class FixedReadableWritableBuffer final : public ReadableStream,
                                              public WritableStream,
                                              public ReadableBuffer,
                                              public WritableBuffer {
        etl::array<uint8_t, MAX_LENGTH> bytes_;
        uint8_t length_;
        _private_fixed::FixedReadableBufferIndex<MAX_LENGTH> read_index_;
        _private_fixed::FixedWritableBufferIndex<MAX_LENGTH> write_index_;

      public:
        FixedReadableWritableBuffer() : length_{MAX_LENGTH} {};
        FixedReadableWritableBuffer(uint8_t length) : length_{length} {};

        inline uint8_t readable_count() const override {
            return read_index_.readable_count(write_index_.index());
        }

        inline uint8_t read() override {
            return read_index_.read(bytes_, write_index_.index());
        }

        inline void read(etl::span<uint8_t> buffer) override {
            read_index_.read(bytes_, write_index_.index(), buffer);
        }

        inline nb::Poll<void> read_all_into(WritableStream &destination) override {
            return read_index_.read_all_into(bytes_, write_index_.index(), destination);
        }

        inline uint8_t writable_count() const override {
            return write_index_.writable_count(length_);
        }

        inline bool write(uint8_t byte) override {
            return write_index_.write(bytes_, length_, byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return write_index_.write(bytes_, length_, buffer);
        }

        inline nb::Poll<void> write_all_from(ReadableStream &source) override {
            return write_index_.write_all_from(bytes_, length_, source);
        }
    };
} // namespace nb::stream
