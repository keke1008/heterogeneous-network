#pragma once

#include <debug_assert.h>
#include <nb/future.h>
#include <nb/stream.h>

namespace net::link {
    class DataWriter final : public nb::stream::WritableStream {
        etl::reference_wrapper<nb::stream::WritableStream> stream_;
        nb::Promise<void> barrier_;
        uint8_t remain_frame_length_;

      public:
        explicit DataWriter(
            etl::reference_wrapper<nb::stream::ReadableWritableStream> stream,
            nb::Promise<void> &&barrier,
            uint8_t length
        )
            : stream_{stream},
              barrier_{etl::move(barrier)},
              remain_frame_length_{length} {}

        inline uint8_t writable_count() const override {
            return etl::min(remain_frame_length_, stream_.get().writable_count());
        }

        inline bool write(uint8_t byte) override {
            DEBUG_ASSERT(remain_frame_length_ > 0, "frame length is zero");
            remain_frame_length_--;
            return stream_.get().write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            DEBUG_ASSERT(remain_frame_length_ >= buffer.size(), "frame length is too short");
            remain_frame_length_ -= buffer.size();
            return stream_.get().write(buffer);
        }

        inline void close() {
            DEBUG_ASSERT(remain_frame_length_ == 0, "written bytes is not enough");
            barrier_.set_value();
        }
    };

    class DataReader final : public nb::stream::ReadableStream {
        etl::reference_wrapper<nb::stream::ReadableStream> stream_;
        uint8_t total_length_;
        nb::Promise<void> barrier_;
        uint8_t remain_frame_length_;

      public:
        explicit DataReader(
            etl::reference_wrapper<nb::stream::ReadableWritableStream> stream,
            nb::Promise<void> &&barrier,
            uint8_t length
        )
            : stream_{stream},
              total_length_{length},
              barrier_{etl::move(barrier)},
              remain_frame_length_{length} {}

        inline uint8_t readable_count() const override {
            return etl::min(remain_frame_length_, stream_.get().readable_count());
        }

        inline uint8_t read() override {
            DEBUG_ASSERT(remain_frame_length_ > 0, "frame length is zero");
            remain_frame_length_--;
            return stream_.get().read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            DEBUG_ASSERT(remain_frame_length_ >= buffer.size(), "frame length is too short");
            remain_frame_length_ -= buffer.size();
            return stream_.get().read(buffer);
        }

        inline void close() {
            DEBUG_ASSERT(remain_frame_length_ == 0, "read bytes is not enough");
            barrier_.set_value();
        }

        inline uint8_t total_length() const {
            return total_length_;
        }
    };
} // namespace net::link
