#pragma once

#include "../event.h"
#include "../frame.h"
#include <net/frame.h>

namespace media::wifi {
    struct ReceivedFrameHeader {
        uint8_t protocol_and_payload_length;
        UdpAddress remote;
        WifiFrameType frame_type;
    };

    class AsyncReceivedFrameHeaderDeserializer {
        nb::de::Dec<uint16_t> length_field_;
        AsyncUdpAddressDeserializer remote_;
        AsyncWifiFrameTypeDeserializer frame_type_;

      public:
        inline ReceivedFrameHeader result() {
            return ReceivedFrameHeader{
                .protocol_and_payload_length =
                    static_cast<uint8_t>(length_field_.result() - WIFI_FRAME_TYPE_SIZE),
                .remote = remote_.result(),
                .frame_type = frame_type_.result(),
            };
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(length_field_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(remote_.deserialize(reader));
            return frame_type_.deserialize(reader);
        }
    };

    class ReceiveDataFrameBody {
        uint8_t payload_length_;
        UdpAddress source_;
        net::frame::AsyncProtocolNumberDeserializer protocol_number_;
        tl::Optional<net::frame::AsyncFrameBufferWriterDeserializer> writer_;

      public:
        explicit ReceiveDataFrameBody(uint8_t protocol_and_payload_length, const UdpAddress &source)
            : payload_length_{static_cast<uint8_t>(
                  protocol_and_payload_length - net::frame::PROTOCOL_SIZE
              )},
              source_{source} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> poll(net::frame::FrameService &fs, R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(protocol_number_.deserialize(readable));

            if (!writer_.has_value()) {
                writer_.emplace(POLL_MOVE_UNWRAP_OR_RETURN(fs.request_frame_writer(payload_length_))
                );
            }

            return writer_->deserialize(readable);
        }

        inline WifiFrame result() const {
            return WifiFrame{
                .protocol_number = protocol_number_.result(),
                .remote = source_,
                .reader = etl::move(*writer_).result().create_reader(),
            };
        }
    };

    class ReceiveDataMessageHandler {
        etl::variant<AsyncReceivedFrameHeaderDeserializer, ReceiveDataFrameBody> task_{};

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        template <nb::AsyncReadable R>
        nb::Poll<etl::optional<WifiEvent>> execute(net::frame::FrameService &fs, R &readable) {
            if (etl::holds_alternative<AsyncReceivedFrameHeaderDeserializer>(task_)) {
                auto &task = etl::get<AsyncReceivedFrameHeaderDeserializer>(task_);
                auto result = POLL_MOVE_UNWRAP_OR_RETURN(task.deserialize(readable));
                if (result != nb::DeserializeResult::Ok) {
                    return etl::optional<WifiEvent>{};
                }

                const auto &header = task.result();
                if (header.frame_type == WifiFrameType::Data) {
                    task_.emplace<ReceiveDataFrameBody>(
                        header.protocol_and_payload_length, header.remote
                    );
                }
            }

            if (etl::holds_alternative<ReceiveDataFrameBody>(task_)) {
                auto &task = etl::get<ReceiveDataFrameBody>(task_);
                if (POLL_UNWRAP_OR_RETURN(task.poll(fs, readable)) != nb::DeserializeResult::Ok) {
                    return etl::optional<WifiEvent>{};
                }
                return etl::optional<WifiEvent>{ReceiveFrame{task.result()}};
            }

            return nb::pending;
        }
    };
} // namespace media::wifi
