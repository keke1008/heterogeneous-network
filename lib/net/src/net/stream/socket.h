#pragma once

#include "./frame.h"
#include <nb/channel.h>
#include <net/frame/service.h>
#include <stdint.h>
#include <util/concepts.h>

namespace net::stream {
    class StreamSenderSocket {
        nb::OneBufferSender<frame::FrameBufferWriter> writer_tx_;
        nb::OneBufferReceiver<frame::FrameBufferReader> written_reader_rx_;

      public:
        StreamSenderSocket() = delete;
        StreamSenderSocket(const StreamSenderSocket &) = delete;
        StreamSenderSocket(StreamSenderSocket &&) = default;
        StreamSenderSocket &operator=(const StreamSenderSocket &) = delete;
        StreamSenderSocket &operator=(StreamSenderSocket &&) = default;

        explicit StreamSenderSocket(
            nb::OneBufferSender<frame::FrameBufferWriter> &&writer_tx,
            nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx
        )
            : writer_tx_{etl::move(writer_tx)},
              written_reader_rx_{etl::move(reader_rx)} {}

        inline bool is_closed() {
            return written_reader_rx_.is_closed();
        }

        // 送信されていないデータがある場合は`nb::Pending`を返す
        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        nb::Poll<void> execute(Requester &requester, Sender &sender) {
            if (writer_tx_.is_closed()) {
                return nb::ready();
            }

            auto reader = POLL_UNWRAP_OR_RETURN(written_reader_rx_.peek());
            POLL_UNWRAP_OR_RETURN(sender.send_frame(reader.get()));
            written_reader_rx_.receive();

            if (writer_tx_.is_sendable()) {
                auto writer = POLL_UNWRAP_OR_RETURN(requester.request_max_length_frame_writer());
                writer_tx_.send(etl::move(writer));
            }

            return nb::pending;
        }
    };

    inline etl::pair<StreamSenderSocket, StreamWriter> make_send_stream() {
        auto [tx1, rx1] = nb::make_one_buffer_channel<frame::FrameBufferWriter>();
        auto [tx2, rx2] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return {
            StreamSenderSocket{etl::move(tx1), etl::move(rx2)},
            StreamWriter{etl::move(rx1), etl::move(tx2)},
        };
    }

    class StreamReceiverSocket {
        nb::OneBufferSender<frame::FrameBufferReader> reader_tx_;

      public:
        StreamReceiverSocket() = delete;
        StreamReceiverSocket(const StreamReceiverSocket &) = delete;
        StreamReceiverSocket(StreamReceiverSocket &&) = default;
        StreamReceiverSocket &operator=(const StreamReceiverSocket &) = delete;
        StreamReceiverSocket &operator=(StreamReceiverSocket &&) = default;

        explicit StreamReceiverSocket(nb::OneBufferSender<frame::FrameBufferReader> &&reader_tx_)
            : reader_tx_{etl::move(reader_tx_)} {}

        template <frame::IFrameReceiver Receiver>
        nb::Poll<void> execute(Receiver &receiver) {
            if (reader_tx_.is_closed()) { // 受信したデータを扱う相手がいない
                while (receiver.receive_frame().is_ready()) {} // 受信したデータを捨てる
                return nb::ready();
            }

            POLL_UNWRAP_OR_RETURN(reader_tx_.poll_sendable());
            reader_tx_.send(POLL_UNWRAP_OR_RETURN(receiver.receive_frame()));
        }

        void close() {
            etl::move(reader_tx_).close();
        }
    };

    inline etl::pair<StreamReceiverSocket, StreamReader> make_receive_stream() {
        auto [tx, rx] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return {StreamReceiverSocket{etl::move(tx)}, StreamReader{etl::move(rx)}};
    }
} // namespace net::stream
