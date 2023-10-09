#pragma once

#include "./pool.h"
#include <nb/stream/fixed.h>

namespace net::frame {
    class FrameBufferReader final : public nb::stream::ReadableStream,
                                    public nb::stream::ReadableBuffer {
        FrameBufferReference buffer_ref_;
        nb::stream::FixedReadableBufferIndex index_;

      public:
        FrameBufferReader() = delete;
        FrameBufferReader(const FrameBufferReader &) = delete;
        FrameBufferReader(FrameBufferReader &&) = default;
        FrameBufferReader &operator=(const FrameBufferReader &) = delete;
        FrameBufferReader &operator=(FrameBufferReader &&) = default;

        explicit FrameBufferReader(FrameBufferReference &&buffer_ref)
            : buffer_ref_{etl::move(buffer_ref)} {}

        inline uint8_t readable_count() const override {
            return index_.readable_count(buffer_ref_.written_count());
        }

        inline uint8_t read() override {
            return index_.read(buffer_ref_.span(), buffer_ref_.written_count());
        }

        inline void read(etl::span<uint8_t> buffer) override {
            index_.read(buffer_ref_.span(), buffer_ref_.written_count(), buffer);
        }

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return index_.read_all_into(
                buffer_ref_.span(), buffer_ref_.written_count(), destination
            );
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.frame_length();
        }

        bool is_buffer_filled() const {
            return buffer_ref_.written_count() == buffer_ref_.frame_length();
        }

        inline bool is_all_read() const {
            return is_buffer_filled() && readable_count() == 0;
        }

        inline etl::span<const uint8_t> written_buffer() const {
            return buffer_ref_.span().subspan(0, buffer_ref_.written_count());
        }

        inline FrameBufferReader make_initial_clone() {
            return FrameBufferReader{buffer_ref_.clone()};
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
            return buffer_ref_.write_index().writable_count(buffer_ref_.frame_length());
        }

        inline bool write(uint8_t byte) override {
            return buffer_ref_.write_index().write(
                buffer_ref_.span(), buffer_ref_.frame_length(), byte
            );
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            return buffer_ref_.write_index().write(
                buffer_ref_.span(), buffer_ref_.frame_length(), buffer
            );
        }

        inline nb::Poll<void> write_all_from(nb::stream::ReadableStream &source) override {
            return buffer_ref_.write_index().write_all_from(
                buffer_ref_.span(), buffer_ref_.frame_length(), source
            );
        }

        inline uint8_t frame_length() const {
            return buffer_ref_.frame_length();
        }

        bool is_buffer_filled() const {
            return buffer_ref_.written_count() == buffer_ref_.frame_length();
        }

        inline void shrink_frame_length_to_fit() {
            buffer_ref_.shrink_frame_length_to_fit();
        }

        inline FrameBufferReader make_initial_reader() {
            return FrameBufferReader{buffer_ref_.clone()};
        }

        template <typename... Args>
        inline void build(Args &&...args) {
            auto span = buffer_ref_.span().subspan(buffer_ref_.written_count());
            nb::buf::build_buffer(span, etl::forward<Args>(args)...);
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
