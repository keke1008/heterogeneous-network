#pragma once

#include "../broker.h"
#include "./frame.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/serde.h>
#include <net/frame/service.h>

namespace net::link::serial {
    struct SkipPreamble {
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &readable) {
            while (true) {
            continue_outer:
                POLL_UNWRAP_OR_RETURN(readable.poll_readable(PREAMBLE_LENGTH));
                for (uint8_t i = 0; i < PREAMBLE_LENGTH; i++) {
                    if (readable.read_unchecked() != PREAMBLE) {
                        goto continue_outer;
                    }
                }

                return nb::DeserializeResult::Ok;
            }
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

        template <nb::AsyncReadable R>
        inline nb::Poll<void> execute(R &readable) {
            while (!frame_writer_.is_all_written()) {
                POLL_UNWRAP_OR_RETURN(readable.poll_readable(1));
                frame_writer_.write_unchecked(readable.read_unchecked());
            }
            return nb::ready();
        }
    };

    class DiscardData {
        nb::de::SkipNBytes discarding_;

      public:
        inline explicit DiscardData(uint8_t length) : discarding_{length} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<void> execute(R &reader) {
            POLL_UNWRAP_OR_RETURN(discarding_.deserialize(reader));
            return nb::ready();
        }
    };

    class FrameReceiver {
        FrameBroker broker_;
        etl::optional<SerialAddress> self_address_;
        etl::variant<
            SkipPreamble,
            AsyncSerialFrameHeaderDeserializer,
            RequestFrameWriter,
            ReceiveData,
            DiscardData>
            state_{};

      public:
        explicit FrameReceiver(const FrameBroker &broker) : broker_{broker} {}

        inline etl::optional<SerialAddress> get_self_address() const {
            return self_address_;
        }

      private:
        template <nb::AsyncReadable R>
        void on_receive_header(R &reader) {
            auto &state = etl::get<AsyncSerialFrameHeaderDeserializer>(state_);
            auto poll_result = state.deserialize(reader);
            if (poll_result.is_pending()) {
                return;
            }
            if (poll_result.unwrap() != nb::DeserializeResult::Ok) {
                state_ = SkipPreamble{};
                return;
            }

            const auto &header = state.result();

            // 最初に受信したフレームの送信先アドレスを自アドレスとする
            if (!self_address_) {
                self_address_ = header.destination;
            }

            if (header.destination != *self_address_) {
                state_ = DiscardData{header.length};
            } else {
                state_ = RequestFrameWriter{header};
            }
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
                .reader = writer.create_reader(),
            });
            state_ = ReceiveData{etl::move(writer)};
        }

      public:
        template <nb::AsyncReadable R>
        void execute(frame::FrameService &fs, R &readable) {
            if (etl::holds_alternative<SkipPreamble>(state_)) {
                if (etl::get<SkipPreamble>(state_).deserialize(readable).is_ready()) {
                    state_.emplace<AsyncSerialFrameHeaderDeserializer>();
                }
            }

            if (etl::holds_alternative<AsyncSerialFrameHeaderDeserializer>(state_)) {
                on_receive_header(readable);
            }

            if (etl::holds_alternative<RequestFrameWriter>(state_)) {
                on_request_frame_writer(fs);
            }

            if (etl::holds_alternative<ReceiveData>(state_)) {
                if (etl::get<ReceiveData>(state_).execute(readable).is_ready()) {
                    state_.emplace<SkipPreamble>();
                }
            }

            if (etl::holds_alternative<DiscardData>(state_)) {
                if (etl::get<DiscardData>(state_).execute(readable).is_ready()) {
                    state_.emplace<SkipPreamble>();
                }
            }
        }
    };
} // namespace net::link::serial
