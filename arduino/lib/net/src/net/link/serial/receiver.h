#pragma once

#include "../broker.h"
#include "./frame.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class ReceivePreamble {
        nb::stream::RepetitionCountWritableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};

      public:
        inline nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            return preamble_.write_all_from(stream);
        }
    };

    class ReceiveHeader {
        nb::stream::FixedWritableBuffer<HEADER_LENGTH> header_;

      public:
        nb::Poll<SerialFrameHeader> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
            return nb::buf::parse<SerialFrameHeaderParser>(header_.written_bytes());
        }
    };

    class RequestFrameWriter {
        SerialFrameHeader header_;

      public:
        inline explicit RequestFrameWriter(const SerialFrameHeader &header) : header_{header} {}

        inline const SerialFrameHeader &header() const {
            return header_;
        }

        inline nb::Poll<frame::FrameBufferWriter> execute(frame::FrameService &service) {
            return service.request_frame_writer(header_.length);
        }
    };

    class ReceiveData {
        frame::FrameBufferWriter frame_writer_;

      public:
        inline explicit ReceiveData(frame::FrameBufferWriter &&frame_writer)
            : frame_writer_{etl::move(frame_writer)} {}

        inline nb::Poll<void>
        execute(frame::FrameService &service, nb::stream::ReadableWritableStream &stream) {
            return frame_writer_.write_all_from(stream);
        }
    };

    class DiscardData {
        nb::stream::DiscardingCountWritableBuffer discarding_;

      public:
        inline explicit DiscardData(uint8_t length) : discarding_{length} {}

        inline nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            return discarding_.write_all_from(stream);
        }
    };

    class FrameReceiver {
        FrameBroker broker_;
        etl::optional<SerialAddress> self_address_;
        etl::variant<ReceivePreamble, ReceiveHeader, RequestFrameWriter, ReceiveData, DiscardData>
            state_{};

      public:
        explicit FrameReceiver(const FrameBroker &broker) : broker_{broker} {}

        inline etl::optional<SerialAddress> get_address() const {
            return self_address_;
        }

      private:
        etl::optional<SerialAddress> on_receive_header(nb::stream::ReadableWritableStream &stream) {
            auto &state = etl::get<ReceiveHeader>(state_);
            const auto &poll_hader = state.execute(stream);
            if (poll_hader.is_pending()) {
                return etl::nullopt;
            }
            const auto &header = poll_hader.unwrap();

            // 最初に受信したフレームの送信先アドレスを自アドレスとする
            etl::optional<SerialAddress> new_self_address;
            if (!self_address_) {
                new_self_address = self_address_ = header.destination;
            }

            if (header.destination != *self_address_) {
                state_ = DiscardData{header.length};
            } else {
                state_ = RequestFrameWriter{header};
            }

            return new_self_address;
        }

        void on_request_frame_writer(frame::FrameService &frame_service) {
            auto &state = etl::get<RequestFrameWriter>(state_);
            auto &&poll_writer = state.execute(frame_service);
            if (poll_writer.is_pending()) {
                return;
            }
            auto &&writer = poll_writer.unwrap();

            const auto &header = state.header();
            broker_.poll_dispatch_received_frame(LinkFrame{
                .protocol_number = header.protocol_number,
                .remote = LinkAddress{header.source},
                .reader = writer.make_initial_reader(),
            });
            state_ = ReceiveData{etl::move(writer)};
        }

      public:
        etl::optional<SerialAddress>
        execute(frame::FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<ReceivePreamble>(state_)) {
                if (etl::get<ReceivePreamble>(state_).execute(stream).is_ready()) {
                    state_ = ReceiveHeader{};
                }
            }

            etl::optional<SerialAddress> new_self_address = etl::nullopt;
            if (etl::holds_alternative<ReceiveHeader>(state_)) {
                new_self_address = on_receive_header(stream);
            }

            if (etl::holds_alternative<RequestFrameWriter>(state_)) {
                on_request_frame_writer(service);
            }

            if (etl::holds_alternative<ReceiveData>(state_)) {
                if (etl::get<ReceiveData>(state_).execute(service, stream).is_ready()) {
                    state_ = ReceivePreamble{};
                }
            }

            if (etl::holds_alternative<DiscardData>(state_)) {
                if (etl::get<DiscardData>(state_).execute(stream).is_ready()) {
                    state_ = ReceivePreamble{};
                }
            }

            return new_self_address;
        }
    };
} // namespace net::link::serial
