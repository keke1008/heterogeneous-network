#pragma once

#include <debug_assert.h>
#include <nb/channel.h>
#include <nb/poll.h>
#include <net/frame/service.h>

namespace net::stream {
    class StreamWriter final : public nb::stream::WritableStream {
        nb::OneBufferReceiver<frame::FrameBufferWriter> writer_rx_;
        nb::OneBufferSender<frame::FrameBufferReader> written_reader_tx_;

        inline const frame::FrameBufferWriter &unwrap_writer() const {
            DEBUG_ASSERT(writer_rx_.is_receivable());
            return writer_rx_.peek().unwrap().get();
        }

        inline frame::FrameBufferWriter &unwrap_writer() {
            DEBUG_ASSERT(writer_rx_.is_receivable());
            return writer_rx_.peek().unwrap().get();
        }

        inline void replace_if_full() {
            DEBUG_ASSERT(!writer_rx_.is_closed());
            if (!writer_rx_.is_receivable()) {
                return;
            }

            auto &writer = writer_rx_.peek().unwrap().get();
            if (writer.writable_count() == 0) {
                auto reader = writer.make_initial_reader();
                if (written_reader_tx_.send(etl::move(reader)).is_ready()) {
                    writer_rx_.receive();
                }
            }
        }

      public:
        StreamWriter() = delete;
        StreamWriter(const StreamWriter &) = delete;
        StreamWriter(StreamWriter &&) = default;
        StreamWriter &operator=(const StreamWriter &) = delete;
        StreamWriter &operator=(StreamWriter &&) = delete;

        explicit StreamWriter(
            nb::OneBufferReceiver<frame::FrameBufferWriter> &&writer_rx,
            nb::OneBufferSender<frame::FrameBufferReader> &&written_reader_tx
        )
            : writer_rx_{etl::move(writer_rx)},
              written_reader_tx_{etl::move(written_reader_tx)} {}

        inline uint8_t writable_count() const override {
            DEBUG_ASSERT(!writer_rx_.is_closed());
            return writer_rx_.is_receivable() ? unwrap_writer().writable_count() : 0;
        }

        inline bool write(uint8_t byte) override {
            DEBUG_ASSERT(writer_rx_.is_receivable());
            bool result = unwrap_writer().write(byte);
            replace_if_full();
            return result;
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            DEBUG_ASSERT(!writer_rx_.is_closed());
            bool result = writer_rx_.is_receivable() && unwrap_writer().write(buffer);
            replace_if_full();
            return result;
        }

        inline void close() {
            etl::move(writer_rx_).close();
            etl::move(written_reader_tx_).close();
        }
    };

    class StreamReader final : public nb::stream::ReadableStream {
        nb::OneBufferReceiver<frame::FrameBufferReader> reader_rx_;

        inline const frame::FrameBufferReader &reader() const {
            DEBUG_ASSERT(reader_rx_.is_receivable());
            return reader_rx_.peek().unwrap().get();
        }

        inline frame::FrameBufferReader &reader() {
            DEBUG_ASSERT(reader_rx_.is_receivable());
            return reader_rx_.peek().unwrap().get();
        }

        inline void replace_if_full() {
            if (!reader_rx_.is_receivable()) {
                return;
            }

            if (reader().readable_count() == 0) {
                reader_rx_.receive();
            }
        }

      public:
        StreamReader() = delete;
        StreamReader(const StreamReader &) = delete;
        StreamReader(StreamReader &&) = default;
        StreamReader &operator=(const StreamReader &) = delete;
        StreamReader &operator=(StreamReader &&) = delete;

        explicit StreamReader(nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx)
            : reader_rx_{etl::move(reader_rx)} {}

        inline uint8_t readable_count() const override {
            return reader_rx_.is_receivable() ? reader().readable_count() : 0;
        }

        inline uint8_t read() override {
            DEBUG_ASSERT(reader_rx_.is_receivable());
            uint8_t byte = reader().read();
            replace_if_full();
            return byte;
        }

        inline void read(etl::span<uint8_t> buffer) override {
            DEBUG_ASSERT(reader_rx_.is_receivable());
            reader().read(buffer);
            replace_if_full();
        }

        inline bool is_closed() const {
            return reader_rx_.is_closed();
        }
    };
} // namespace net::stream
