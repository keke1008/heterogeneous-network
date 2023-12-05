#pragma once

#include "../broker.h"
#include "./frame.h"
#include <nb/serde.h>
#include <net/frame.h>

namespace net::link::serial {
    class AsyncPreambleSerializer {
        bool done_{false};

      public:
        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            if (done_) {
                return nb::ser::SerializeResult::Ok;
            }

            POLL_UNWRAP_OR_RETURN(writable.poll_writable(PREAMBLE_LENGTH));
            for (uint8_t i = 0; i < PREAMBLE_LENGTH; i++) {
                writable.write_unchecked(PREAMBLE);
            }

            done_ = true;
            return nb::ser::SerializeResult::Ok;
        }
    };

    class AsyncFrameSerializer {
        AsyncPreambleSerializer preamble_;
        AsyncSerialFrameHeaderSerializer header_;
        frame::AsyncFrameBufferReaderSerializer reader_;

      public:
        explicit AsyncFrameSerializer(
            const SerialFrameHeader &header,
            frame::FrameBufferReader &&reader
        )
            : header_{header},
              reader_{etl::move(reader)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            POLL_UNWRAP_OR_RETURN(preamble_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(header_.serialize(writable));
            return reader_.serialize(writable);
        }
    };

    class FrameSender {
        FrameBroker broker_;
        etl::optional<AsyncFrameSerializer> frame_serializer_;

      public:
        explicit FrameSender(const FrameBroker &broker) : broker_{broker} {}

        template <nb::AsyncWritable W>
        inline void execute(W &writable, SerialAddress &self_address) {
            if (!frame_serializer_) {
                auto poll_frame = broker_.poll_get_send_requested_frame(AddressType::Serial);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                const auto &header = SerialFrameHeader{
                    .protocol_number = frame.protocol_number,
                    .source = self_address,
                    .destination = SerialAddress{frame.remote},
                    .length = frame.reader.buffer_length(),
                };
                frame_serializer_.emplace(header, etl::move(frame.reader));
            }

            if (frame_serializer_->serialize(writable).is_ready()) {
                frame_serializer_.reset();
            }
        }
    };
} // namespace net::link::serial
