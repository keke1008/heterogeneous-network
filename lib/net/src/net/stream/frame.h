#pragma once

#include <debug_assert.h>
#include <memory/pair_shared.h>
#include <nb/poll.h>
#include <net/frame/service.h>

namespace net::stream {
    class StreamWriter final : public nb::stream::WritableStream {
        memory::Reference<etl::optional<frame::FrameBufferWriter>> writer_;

        inline const etl::optional<frame::FrameBufferWriter> &writer() const {
            DEBUG_ASSERT(writer_.has_pair());
            return writer_.get()->get();
        }

        inline etl::optional<frame::FrameBufferWriter> &writer() {
            DEBUG_ASSERT(writer_.has_pair());
            return writer_.get()->get();
        }

      public:
        StreamWriter() = delete;
        StreamWriter(const StreamWriter &) = delete;
        StreamWriter(StreamWriter &&) = default;
        StreamWriter &operator=(const StreamWriter &) = delete;
        StreamWriter &operator=(StreamWriter &&) = delete;

        inline ~StreamWriter() {
            writer_.unpair();
        }

        explicit StreamWriter(memory::Reference<etl::optional<frame::FrameBufferWriter>> &&writer)
            : writer_{etl::move(writer)} {}

        inline uint8_t writable_count() const override {
            if (!writer_.has_pair()) { // 送信する相手がいない
                return 255;            // 書き込まれたデータは破棄される
            }
            return writer().has_value() ? writer()->writable_count() : 0;
        }

        inline bool write(uint8_t byte) override {
            if (!writer_.has_pair()) { // 送信する相手がいない
                return false;          // 書き込まれたデータは破棄される
            }
            DEBUG_ASSERT(writer().has_value());
            return writer()->write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            if (!writer_.has_pair()) { // 送信する相手がいない
                return false;          // 書き込まれたデータは破棄される
            }
            return writer().has_value() && writer()->write(buffer);
        }

        inline void close() {
            writer_.unpair();
        }
    };

    class StreamReader final : public nb::stream::ReadableStream {
        memory::Owned<etl::optional<frame::FrameBufferReader>> reader_;

        inline const etl::optional<frame::FrameBufferReader> &reader() const {
            DEBUG_ASSERT(reader_.has_pair());
            return reader_.get();
        }

        inline etl::optional<frame::FrameBufferReader> &reader() {
            DEBUG_ASSERT(reader_.has_pair());
            return reader_.get();
        }

      public:
        StreamReader() = delete;
        StreamReader(const StreamReader &) = delete;
        StreamReader(StreamReader &&) = default;
        StreamReader &operator=(const StreamReader &) = delete;
        StreamReader &operator=(StreamReader &&) = delete;

        explicit StreamReader(memory::Owned<etl::optional<frame::FrameBufferReader>> reader)
            : reader_{etl::move(reader)} {}

        inline uint8_t readable_count() const override {
            return reader().has_value() ? reader()->readable_count() : 0;
        }

        inline uint8_t read() override {
            DEBUG_ASSERT(reader().has_value());
            return reader()->read();
        }

        inline void read(etl::span<uint8_t> buffer) override {
            DEBUG_ASSERT(reader().has_value());
            return reader()->read(buffer);
        }

        inline bool is_closed() const {
            return !reader_.has_pair();
        }
    };
} // namespace net::stream
