#pragma once

#include "../address.h"
#include "../media.h"
#include "./address.h"
#include "./layout.h"
#include <nb/buf.h>
#include <nb/stream.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class CreateFrameBuffer {
        FrameHeader header_;

      public:
        inline explicit CreateFrameBuffer(const FrameHeader &header) : header_{header} {}

        template <net::frame::IFrameService FrameService>
        inline nb::Poll<frame::FrameBufferWriter> execute(FrameService &service) {
            auto writer = POLL_MOVE_UNWRAP_OR_RETURN(service.request_frame_writer(header_.length));
            writer.build(header_);
            return etl::move(writer);
        }
    };

    class SendFrame {
        nb::stream::RepetitionReadableBuffer preamble_{PREAMBLE, PREAMBLE_LENGTH};
        nb::stream::FixedReadableBuffer<HEADER_LENGTH> header_;
        frame::FrameBufferReader reader_;

      public:
        explicit SendFrame(const FrameHeader &header, frame::FrameBufferReader &&reader)
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
        etl::optional<SendFrame> sender_;
        SerialAddress self_address_;

      public:
        explicit FrameSender(SerialAddress self_address) : self_address_{self_address} {}

        inline nb::Poll<void> send_frame(link::Frame &&frame) {
            if (sender_.has_value()) {
                return nb::pending;
            } else {
                sender_ = SendFrame{
                    FrameHeader{
                        .protocol_number = frame.protocol_number,
                        .source = SerialAddress{self_address_},
                        .destination = SerialAddress{frame.peer},
                        .length = frame.length,
                    },
                    etl::move(frame.reader)};
                return nb::ready();
            }
        }

        inline void execute(nb::stream::ReadableWritableStream &stream) {
            if (sender_.has_value()) {
                auto poll = sender_.value().execute(stream);
                if (poll.is_ready()) {
                    sender_ = etl::nullopt;
                }
            }
        }
    };
} // namespace net::link::serial
