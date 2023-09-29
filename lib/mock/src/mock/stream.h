#pragma once

#include <nb/stream.h>

namespace mock {
    struct MockReadableWritableStream : public nb::stream::ReadableWritableStream,
                                        public nb::stream::ReadableWritableBuffer {
        nb::stream::FixedReadableWritableBuffer<255> read_buffer_;
        nb::stream::FixedReadableWritableBuffer<255> write_buffer_;

        inline uint8_t readable_count() const override {
            return read_buffer_.readable_count();
        }

        inline uint8_t read() override {
            return read_buffer_.read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            read_buffer_.read(buffer);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return read_buffer_.read_all_into(destination);
        }

        inline uint8_t writable_count() const override {
            return write_buffer_.writable_count();
        }

        inline bool write(uint8_t byte) override {
            return write_buffer_.write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return write_buffer_.write(buffer);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return write_buffer_.write_all_from(source);
        }

        template <typename Iterable>
        [[deprecated("Use read_buffer_.write_str() instead")]] inline void
        write_to_read_buffer(Iterable &&iterable) {
            for (auto byte : iterable) {
                read_buffer_.write(byte);
            }
        }

        [[deprecated("Use read_buffer_.write_str() instead")]] inline void
        write_to_read_buffer(etl::string_view str) {
            for (uint8_t byte : str) {
                read_buffer_.write(byte);
            }
        }

        [[deprecated("Use write_buffer_.written_buffer() and util::as_str() instead")]] inline bool
        consume_write_buffer_and_equals_to(etl::span<const uint8_t> span) {
            if (write_buffer_.readable_count() != span.size()) {
                return false;
            }
            for (auto byte : span) {
                if (write_buffer_.read() != byte) {
                    return false;
                }
            }
            return true;
        }

        [[deprecated("Use write_buffer_.written_buffer() and util::as_str() instead")]] inline bool
        consume_write_buffer_and_equals_to(etl::string_view str) {
            if (write_buffer_.readable_count() != str.size()) {
                return false;
            }
            for (auto byte : str) {
                if (write_buffer_.read() != *reinterpret_cast<uint8_t *>(&byte)) {
                    return false;
                }
            }
            return true;
        }
    };
} // namespace mock
