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
    };
} // namespace mock
