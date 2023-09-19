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

        inline bool write(etl::span<uint8_t> buffer) override {
            return write_buffer_.write(buffer);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return write_buffer_.write_all_from(source);
        }

        template <typename Iterable>
        inline void write_to_read_buffer(Iterable &&iterable) {
            for (auto byte : iterable) {
                read_buffer_.write(byte);
            }
        }

        inline etl::optional<uint8_t> read_from_write_buffer() {
            if (write_buffer_.readable_count() == 0) {
                return etl::nullopt;
            }
            return write_buffer_.read();
        }
    };
} // namespace mock
