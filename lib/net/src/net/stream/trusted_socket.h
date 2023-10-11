#pragma once

#include "./socket.h"
#include <net/frame/service.h>
#include <net/trusted/socket.h>

namespace net::stream {
    class TrustedStreamSenderSocket {
        trusted::SenderSocket trusted_socket_;
        stream::StreamSenderSocket stream_socket_;
        bool done_ = false;

      public:
        TrustedStreamSenderSocket() = delete;
        TrustedStreamSenderSocket(const TrustedStreamSenderSocket &) = delete;
        TrustedStreamSenderSocket(TrustedStreamSenderSocket &&) = default;
        TrustedStreamSenderSocket &operator=(const TrustedStreamSenderSocket &) = delete;
        TrustedStreamSenderSocket &operator=(TrustedStreamSenderSocket &&) = default;

        explicit TrustedStreamSenderSocket(stream::StreamSenderSocket &&stream_socket)
            : stream_socket_{etl::move(stream_socket)} {}

        template <frame::IFrameBufferRequester Requester, frame::IFrameSender Sender>
        inline nb::Poll<void> execute(Requester &requester, Sender &sender) {
            if (done_) {
                return nb::ready();
            }

            auto trusted_requester = trusted_socket_.get_buffer_requester(requester);
            if (stream_socket_.execute(trusted_requester, trusted_socket_).is_ready()) {
                done_ = true;
                return nb::ready();
            }

            if (trusted_socket_.execute(requester, sender).is_ready()) {
                done_ = true;
                return nb::ready();
            }

            return nb::pending;
        }
    };

    inline etl::pair<TrustedStreamSenderSocket, stream::StreamWriter> make_send_socket() {
        auto [stream_socket, writer] = stream::make_send_stream();
        return {TrustedStreamSenderSocket{etl::move(stream_socket)}, etl::move(writer)};
    }

    class TrustedStreamReceiverSocket {
        trusted::ReceiverSocket trusted_socket_;
        stream::StreamReceiverSocket stream_socket_;
        bool done_ = false;

      public:
        TrustedStreamReceiverSocket() = delete;
        TrustedStreamReceiverSocket(const TrustedStreamReceiverSocket &) = delete;
        TrustedStreamReceiverSocket(TrustedStreamReceiverSocket &&) = default;
        TrustedStreamReceiverSocket &operator=(const TrustedStreamReceiverSocket &) = delete;
        TrustedStreamReceiverSocket &operator=(TrustedStreamReceiverSocket &&) = default;

        explicit TrustedStreamReceiverSocket(stream::StreamReceiverSocket &&stream_socket)
            : stream_socket_{etl::move(stream_socket)} {}

        template <frame::IFrameReceiver Receiver>
        inline nb::Poll<void> execute(Receiver &receiver) {
            if (done_) {
                return nb::ready();
            }

            if (stream_socket_.execute(trusted_socket_).is_ready()) {
                done_ = true;
                return nb::ready();
            }

            if (trusted_socket_.execute(receiver).is_ready()) {
                done_ = true;
                return nb::ready();
            }

            return nb::pending;
        }
    };

    inline etl::pair<TrustedStreamReceiverSocket, stream::StreamReader> make_receive_socket() {
        auto [stream_socket, reader] = stream::make_receive_stream();
        return {TrustedStreamReceiverSocket{etl::move(stream_socket)}, etl::move(reader)};
    }
} // namespace net::stream
