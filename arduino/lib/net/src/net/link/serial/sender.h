#pragma once

#include "../broker.h"
#include "./frame.h"
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SendFrame {
        nb::stream::RepetitionReadableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        nb::stream::FixedReadableBuffer<HEADER_LENGTH> header_;
        frame::FrameBufferReader reader_;

      public:
        explicit SendFrame(const SerialFrameHeader &header, frame::FrameBufferReader &&reader)
            : header_{header},
              reader_{etl::move(reader)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(preamble_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(header_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(reader_.read_all_into(stream));
            return nb::ready();
        }
    };

    class FrameSender {
        memory::StaticRef<FrameBroker> broker_;
        etl::optional<SendFrame> sender_;
        etl::optional<SerialAddress> self_address_;

      public:
        explicit FrameSender(const memory::StaticRef<FrameBroker> &broker) : broker_{broker} {}

        inline void set_self_address_if_not_set(SerialAddress address) {
            if (!self_address_) {
                self_address_ = address;
            }
        }

        inline void execute(nb::stream::ReadableWritableStream &stream) {
            if (!self_address_) {
                return;
            }

            if (!sender_) {
                auto poll_frame = broker_->poll_get_send_requested_frame(AddressType::Serial);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                const auto &header = SerialFrameHeader{
                    .protocol_number = frame.protocol_number,
                    .source = *self_address_,
                    .destination = SerialAddress{frame.remote},
                    .length = frame.reader.readable_count(),
                };
                sender_ = SendFrame{header, etl::move(frame.reader)};
            }

            if (sender_ && sender_.value().execute(stream).is_ready()) {
                sender_ = etl::nullopt;
            }
        }
    };
} // namespace net::link::serial
