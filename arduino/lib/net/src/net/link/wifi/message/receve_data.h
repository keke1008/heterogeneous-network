#pragma once

#include "../../media.h"
#include "../frame.h"
#include <logger.h>
#include <nb/buf.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    struct ReceivedFrameHeader {
        frame::ProtocolNumber protocol_number;
        uint8_t length;
        WifiAddress remote;
    };

    class AsyncReceivedFrameHeaderDeserializer {
        nb::de::Dec<uint8_t> length_;
        AsyncWifiAddressDeserializer remote_;
        nb::de::Bin<uint8_t> protocol_;

      public:
        inline ReceivedFrameHeader result() {
            return ReceivedFrameHeader{
                .protocol_number = static_cast<frame::ProtocolNumber>(protocol_.result()),
                .length = length_.result(),
                .remote = remote_.result(),
            };
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(remote_.deserialize(reader));
            return protocol_.deserialize(reader);
        }
    };

    class CreateFrameWriter {
        uint8_t length_;

      public:
        explicit CreateFrameWriter(uint8_t length) : length_{length} {}

        inline nb::Poll<frame::FrameBufferWriter> poll(frame::FrameService &frame_service) {
            return frame_service.request_frame_writer(length_);
        }
    };

    class ReceiveBody {
        frame::FrameBufferWriter writer_;

      public:
        explicit ReceiveBody(frame::FrameBufferWriter &&writer) : writer_{etl::move(writer)} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<frame::FrameBufferReader> poll(R &readable) {
            while (!writer_.is_all_written()) {
                POLL_UNWRAP_OR_RETURN(readable.poll_readable(1));
                writer_.write(readable.read_unchecked());
            }
            return writer_.unwrap_reader();
        }
    };

    class ReceiveDataMessageHandler {
        etl::variant<AsyncReceivedFrameHeaderDeserializer, CreateFrameWriter, ReceiveBody> task_{};
        etl::optional<ReceivedFrameHeader> header_;

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        template <nb::AsyncReadable R>
        nb::Poll<WifiFrame> execute(frame::FrameService &service, R &readable) {
            if (etl::holds_alternative<AsyncReceivedFrameHeaderDeserializer>(task_)) {
                auto &task = etl::get<AsyncReceivedFrameHeaderDeserializer>(task_);
                POLL_MOVE_UNWRAP_OR_RETURN(task.deserialize(readable));
                header_ = task.result();
                uint8_t length = header_->length - frame::PROTOCOL_SIZE;
                task_.emplace<CreateFrameWriter>(length);
            }

            if (etl::holds_alternative<CreateFrameWriter>(task_)) {
                auto &task = etl::get<CreateFrameWriter>(task_);
                auto writer = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(service));
                task_.emplace<ReceiveBody>(etl::move(writer));
            }

            if (etl::holds_alternative<ReceiveBody>(task_)) {
                auto &task = etl::get<ReceiveBody>(task_);
                auto reader = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(readable));
                return WifiFrame{
                    .protocol_number = header_->protocol_number,
                    .remote = header_->remote,
                    .reader = etl::move(reader),
                };
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
