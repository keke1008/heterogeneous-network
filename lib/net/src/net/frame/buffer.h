#pragma once

#include "./pool.h"

namespace net::frame {
    class FrameBufferReader final : public nb::stream::ReadableStream,
                                    public nb::stream::ReadableBuffer {
        FrameBufferReference buffer_ref_;

      public:
        FrameBufferReader() = delete;
        FrameBufferReader(const FrameBufferReader &) = delete;
        FrameBufferReader(FrameBufferReader &&) = default;
        FrameBufferReader &operator=(const FrameBufferReader &) = delete;
        FrameBufferReader &operator=(FrameBufferReader &&) = default;

        FrameBufferReader(FrameBufferReference &&buffer_ref) : buffer_ref_{etl::move(buffer_ref)} {}

        inline uint8_t readable_count() const override {
            return buffer_ref_.buffer().readable_count();
        }

        inline uint8_t read() override {
            return buffer_ref_.buffer().read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            buffer_ref_.buffer().read(buffer);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return buffer_ref_.buffer().read_all_into(destination);
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.buffer().length();
        }
    };

    class FrameBufferWriter final : public nb::stream::WritableStream,
                                    public nb::stream::WritableBuffer {
        FrameBufferReference buffer_ref_;

      public:
        FrameBufferWriter() = delete;
        FrameBufferWriter(const FrameBufferWriter &) = delete;
        FrameBufferWriter(FrameBufferWriter &&) = default;
        FrameBufferWriter &operator=(const FrameBufferWriter &) = delete;
        FrameBufferWriter &operator=(FrameBufferWriter &&) = default;

        FrameBufferWriter(FrameBufferReference &&buffer_ref) : buffer_ref_{etl::move(buffer_ref)} {}

        inline uint8_t writable_count() const override {
            return buffer_ref_.buffer().writable_count();
        }

        inline bool write(uint8_t byte) override {
            return buffer_ref_.buffer().write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return buffer_ref_.buffer().write(buffer);
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return buffer_ref_.buffer().write_all_from(source);
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.buffer().length();
        }
    };

    inline etl::pair<FrameBufferReader, FrameBufferWriter>
    make_frame_buffer_pair(FrameBufferReference &&buffer_ref) {
        return etl::pair{
            FrameBufferReader{etl::move(buffer_ref.clone())},
            FrameBufferWriter{etl::move(buffer_ref)},
        };
    }
} // namespace net::frame
