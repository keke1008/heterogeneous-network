#pragma once

#include "./frame.h"
#include <nb/serde.h>
#include <net/frame.h>
#include <net/link.h>

namespace media::serial {
    class AsyncPreambleSerializer {
        bool done_{false};

      public:
        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            if (done_) {
                return nb::ser::SerializeResult::Ok;
            }

            POLL_UNWRAP_OR_RETURN(writable.poll_writable(PREAMBLE_LENGTH + LAST_PREAMBLE_LENGTH));
            for (uint8_t i = 0; i < PREAMBLE_LENGTH; i++) {
                writable.write_unchecked(PREAMBLE);
            }

            for (uint8_t i = 0; i < LAST_PREAMBLE_LENGTH; i++) {
                writable.write_unchecked(LAST_PREAMBLE);
            }

            done_ = true;
            return nb::ser::SerializeResult::Ok;
        }
    };

    class AsyncFrameSerializer {
        AsyncPreambleSerializer preamble_;
        AsyncSerialFrameHeaderSerializer header_;
        net::frame::AsyncFrameBufferReaderSerializer reader_;

      public:
        explicit AsyncFrameSerializer(
            const SerialFrameHeader &header,
            net::frame::FrameBufferReader &&reader
        )
            : header_{header},
              reader_{etl::move(reader)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            SERDE_SERIALIZE_OR_RETURN(preamble_.serialize(writable));
            SERDE_SERIALIZE_OR_RETURN(header_.serialize(writable));
            return reader_.serialize(writable);
        }
    };

    class FrameSender {
        memory::Static<net::link::FrameBroker> &broker_;
        etl::optional<AsyncFrameSerializer> frame_serializer_;

      public:
        explicit FrameSender(memory::Static<net::link::FrameBroker> &broker) : broker_{broker} {}

        template <nb::AsyncWritable W>
        inline void execute(
            W &writable,
            SerialAddress &self_address,
            etl::optional<SerialAddress> remote_address
        ) {
            while (!frame_serializer_) {
                auto poll_frame =
                    broker_->poll_get_send_requested_frame(net::link::AddressType::Serial);
                if (poll_frame.is_pending()) {
                    return;
                }

                auto &&frame = poll_frame.unwrap();
                if (!SerialAddress::is_convertible_address(frame.remote)) {
                    continue;
                }

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
} // namespace media::serial
