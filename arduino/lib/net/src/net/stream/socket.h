#pragma once

#include "./frame.h"
#include <nb/channel.h>
#include <net/frame/service.h>
#include <net/socket.h>
#include <stdint.h>
#include <util/concepts.h>

namespace net::stream {
    template <socket::ISenderSocket LowerSenderSocket>
    class SenderSocket {
        LowerSenderSocket sender_;
        nb::OneBufferSender<frame::FrameBufferWriter> writer_tx_;
        nb::OneBufferReceiver<frame::FrameBufferReader> written_reader_rx_;

      public:
        SenderSocket() = delete;
        SenderSocket(const SenderSocket &) = delete;
        SenderSocket(SenderSocket &&) = default;
        SenderSocket &operator=(const SenderSocket &) = delete;
        SenderSocket &operator=(SenderSocket &&) = default;

        explicit SenderSocket(
            LowerSenderSocket &&sender,
            nb::OneBufferSender<frame::FrameBufferWriter> &&writer_tx,
            nb::OneBufferReceiver<frame::FrameBufferReader> &&reader_rx
        )
            : sender_{etl::move(sender)},
              writer_tx_{etl::move(writer_tx)},
              written_reader_rx_{etl::move(reader_rx)} {}

        inline bool is_closed() {
            return written_reader_rx_.is_closed();
        }

      private:
        nb::Poll<void> execute_inner() {
            if (writer_tx_.is_closed()) {
                return nb::ready();
            }

            auto reader = POLL_UNWRAP_OR_RETURN(written_reader_rx_.peek());
            POLL_UNWRAP_OR_RETURN(sender_.send_frame(reader.get()));
            written_reader_rx_.receive();

            if (writer_tx_.is_sendable()) {
                auto writer = POLL_UNWRAP_OR_RETURN(sender_.request_max_length_frame_writer());
                writer_tx_.send(etl::move(writer));
            }

            return nb::pending;
        }

      public:
        // 送信されていないデータがある場合は`nb::Pending`を返す
        nb::Poll<void> execute() {
            auto result = execute_inner();
            if (sender_.execute().is_ready()) {
                return nb::ready();
            }
            return result;
        }
    };

    template <socket::ISenderSocket LowerSenderSocket>
    inline etl::pair<SenderSocket<LowerSenderSocket>, StreamWriter> make_send_stream() {
        auto [tx1, rx1] = nb::make_one_buffer_channel<frame::FrameBufferWriter>();
        auto [tx2, rx2] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return {
            SenderSocket<LowerSenderSocket>{etl::move(tx1), etl::move(rx2)},
            StreamWriter{etl::move(rx1), etl::move(tx2)},
        };
    }

    template <socket::IReceiverSocket LowerReceiverSocket>
    class ReceiverSocket {
        LowerReceiverSocket receiver_;
        nb::OneBufferSender<frame::FrameBufferReader> reader_tx_;

      public:
        ReceiverSocket() = delete;
        ReceiverSocket(const ReceiverSocket &) = delete;
        ReceiverSocket(ReceiverSocket &&) = default;
        ReceiverSocket &operator=(const ReceiverSocket &) = delete;
        ReceiverSocket &operator=(ReceiverSocket &&) = default;

        explicit ReceiverSocket(
            LowerReceiverSocket &&receiver,
            nb::OneBufferSender<frame::FrameBufferReader> &&reader_tx_
        )
            : receiver_{etl::move(receiver)},
              reader_tx_{etl::move(reader_tx_)} {}

        nb::Poll<void> execute() {
            if (receiver_.execute().is_ready()) {
                return nb::ready();
            }

            if (reader_tx_.is_closed()) { // 受信したデータを扱う相手がいない
                while (receiver_.receive_frame().is_ready()) {} // 受信したデータを捨てる
                return nb::ready();
            }

            POLL_UNWRAP_OR_RETURN(reader_tx_.poll_sendable());
            reader_tx_.send(POLL_UNWRAP_OR_RETURN(receiver_.receive_frame()));
        }

        void close() {
            etl::move(reader_tx_).close();
        }
    };

    template <socket::IReceiverSocket LowerReceiverSocket>
    inline etl::pair<ReceiverSocket<LowerReceiverSocket>, StreamReader> make_receive_stream() {
        auto [tx, rx] = nb::make_one_buffer_channel<frame::FrameBufferReader>();
        return {
            ReceiverSocket<LowerReceiverSocket>{etl::move(tx)},
            StreamReader{etl::move(rx)},
        };
    }
} // namespace net::stream
