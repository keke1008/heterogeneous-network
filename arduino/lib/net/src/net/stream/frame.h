#pragma once

#include <logger.h>
#include <nb/channel.h>
#include <nb/poll.h>
#include <net/frame/service.h>

namespace net::stream {
    class StreamWriter final : public nb::stream::WritableStream {
        nb::OneBufferReceiver<frame::FrameBufferWriter> writer_rx_;
        nb::OneBufferSender<frame::FrameBufferReader> written_reader_tx_;

        inline const frame::FrameBufferWriter &unwrap_writer() const {
            ASSERT(writer_rx_.is_receivable());
            return writer_rx_.peek().unwrap().get();
        }

        inline frame::FrameBufferWriter &unwrap_writer() {
            ASSERT(writer_rx_.is_receivable());
            return writer_rx_.peek().unwrap().get();
        }

      public:
        StreamWriter() = delete;
        StreamWriter(const StreamWriter &) = delete;
        StreamWriter(StreamWriter &&) = default;
        StreamWriter &operator=(const StreamWriter &) = delete;
        StreamWriter &operator=(StreamWriter &&) = default;

        explicit StreamWriter(
            nb::OneBufferReceiver<frame::FrameBufferWriter> &&writer_rx,
            nb::OneBufferSender<frame::FrameBufferReader> &&written_reader_tx
        )
            : writer_rx_{etl::move(writer_rx)},
              written_reader_tx_{etl::move(written_reader_tx)} {}

        inline uint8_t writable_count() const override {
            ASSERT(!writer_rx_.is_closed());
            return writer_rx_.is_receivable() ? unwrap_writer().writable_count() : 0;
        }

        inline bool write(uint8_t byte) override {
            ASSERT(writer_rx_.is_receivable());
            return unwrap_writer().write(byte);
        }

        inline bool write(etl::span<const uint8_t> buffer) override {
            ASSERT(!writer_rx_.is_closed());
            return writer_rx_.is_receivable() && unwrap_writer().write(buffer);
        }

        template <nb::buf::IBufferWriter... Writers>
        inline bool write(Writers &&...writers) {
            ASSERT(!writer_rx_.is_closed());
            if (writer_rx_.is_receivable()) {
                auto &writer = writer_rx_.peek().unwrap().get();
                return writer.write(etl::forward<Writers>(writers)...);
            }
        }

        inline void request_next_frame() {
            ASSERT(!writer_rx_.is_closed());
            if (writer_rx_.is_receivable() && written_reader_tx_.is_sendable()) {
                auto writer = etl::move(writer_rx_.receive().unwrap());
                writer.shrink_frame_length_to_fit();
                written_reader_tx_.send(etl::move(writer.make_initial_reader()));
            }
        }

        inline void close() {
            etl::move(writer_rx_).close();
            etl::move(written_reader_tx_).close();
        }
    };

    class StreamReader final : public nb::stream::ReadableStream {
        nb::OneBufferReceiver<frame::FrameBufferReader> reader_rx_;

        inline const frame::FrameBufferReader &reader() const {
            ASSERT(reader_rx_.is_receivable());
            return reader_rx_.peek().unwrap().get();
        }

        inline frame::FrameBufferReader &reader() {
            ASSERT(reader_rx_.is_receivable());
            return reader_rx_.peek().unwrap().get();
        }

        inline void replace_if_empty() {
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
        StreamReader &operator=(StreamReader &&) = default;

        explicit StreamReader(nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx)
            : reader_rx_{etl::move(reader_rx)} {}

        inline uint8_t readable_count() const override {
            return reader_rx_.is_receivable() ? reader().readable_count() : 0;
        }

        inline uint8_t read() override {
            ASSERT(reader_rx_.is_receivable());
            uint8_t byte = reader().read();
            replace_if_empty();
            return byte;
        }

        inline void read(etl::span<uint8_t> buffer) override {
            ASSERT(reader_rx_.is_receivable());
            reader().read(buffer);
            replace_if_empty();
        }

        template <nb::buf::IBufferParser Parser>
        inline decltype(auto) read() {
            ASSERT(reader_rx_.is_receivable());
            decltype(auto) result = reader().read<Parser>();
            replace_if_empty();
            return result;
        }

        inline bool is_buffer_filled() const {
            return reader_rx_.is_receivable() && reader().is_buffer_filled();
        }

        inline bool is_closed() const {
            return reader_rx_.is_closed();
        }
    };
} // namespace net::stream
