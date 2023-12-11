#pragma once

#include "../event.h"
#include "../frame.h"
#include <net/frame.h>

namespace net::link::wifi {
    struct ReceivedFrameHeader {
        uint8_t length;
        WifiAddress remote;
        WifiFrameType frame_type;
    };

    class AsyncReceivedFrameHeaderDeserializer {
        nb::de::Dec<uint8_t> length_;
        AsyncWifiAddressDeserializer remote_;
        nb::de::Bin<uint8_t> frame_type_;

      public:
        inline ReceivedFrameHeader result() {
            return ReceivedFrameHeader{
                .length = length_.result(),
                .remote = remote_.result(),
                .frame_type = static_cast<WifiFrameType>(frame_type_.result()),
            };
        }

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            SERDE_DESERIALIZE_OR_RETURN(length_.deserialize(reader));
            SERDE_DESERIALIZE_OR_RETURN(remote_.deserialize(reader));
            return frame_type_.deserialize(reader);
        }
    };

    class ReceiveControlFrameBody {
        using Union = nb::de::Union<nb::de::Empty, AsyncWifiAddressDeserializer>;

        WifiAddress source_;
        Union union_;

        static inline Union create_union(WifiFrameType frame_type) {
            switch (frame_type) {
            case WifiFrameType::GlobalAddressRequest:
                return Union{nb::de::Empty{}};
            case WifiFrameType::GlobalAddressResponse:
                return Union{AsyncWifiAddressDeserializer{}};
            default:
                UNREACHABLE_DEFAULT_CASE;
            }
        }

      public:
        explicit ReceiveControlFrameBody(const WifiAddress &source, WifiFrameType frame_type)
            : source_{source},
              union_{create_union(frame_type)} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> poll(R &readable) {
            return union_.deserialize(readable);
        }

        inline WifiEvent result() const {
            auto &&frame = etl::visit<WifiControlFrame>(
                util::Visitor{
                    [&](const nb::EmptyMarker &) { return WifiGlobalAddressRequestFrame{}; },
                    [&](const WifiAddress &address) {
                        return WifiGlobalAddressResponseFrame{.address = address};
                    }
                },
                union_.result()
            );
            return ReceiveControlFrame{.source = source_, .frame = frame};
        }
    };

    class ReceiveDataFrameBody {
        uint8_t length_;
        WifiAddress source_;
        frame::AsyncProtocolNumberDeserializer protocol_number_;
        tl::Optional<frame::FrameBufferWriter> writer_;

      public:
        explicit ReceiveDataFrameBody(uint8_t length, const WifiAddress &source)
            : length_{length},
              source_{source} {}

        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> poll(frame::FrameService &fs, R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(protocol_number_.deserialize(readable));

            if (!writer_.has_value()) {
                writer_ = POLL_MOVE_UNWRAP_OR_RETURN(fs.request_frame_writer(length_));
            }

            while (!writer_->is_all_written()) {
                POLL_UNWRAP_OR_RETURN(readable.poll_readable(1));
                writer_->write(readable.read_unchecked());
            }

            return nb::DeserializeResult::Ok;
        }

        inline WifiDataFrame result() const {
            return WifiDataFrame{
                .protocol_number = protocol_number_.result(),
                .remote = source_,
                .reader = writer_->create_reader(),
            };
        }
    };

    class ReceiveDataMessageHandler {
        etl::variant<
            AsyncReceivedFrameHeaderDeserializer,
            ReceiveDataFrameBody,
            ReceiveControlFrameBody>
            task_{};

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        template <nb::AsyncReadable R>
        nb::Poll<etl::optional<WifiEvent>> execute(frame::FrameService &fs, R &readable) {
            if (etl::holds_alternative<AsyncReceivedFrameHeaderDeserializer>(task_)) {
                auto &task = etl::get<AsyncReceivedFrameHeaderDeserializer>(task_);
                POLL_MOVE_UNWRAP_OR_RETURN(task.deserialize(readable));

                const auto &header = task.result();
                uint8_t length = header.length - frame::PROTOCOL_SIZE;
                if (header.frame_type == WifiFrameType::Data) {
                    task_.emplace<ReceiveDataFrameBody>(length, header.remote);
                } else {
                    task_.emplace<ReceiveControlFrameBody>(header.remote, header.frame_type);
                }
            }

            if (etl::holds_alternative<ReceiveDataFrameBody>(task_)) {
                auto &task = etl::get<ReceiveDataFrameBody>(task_);
                if (POLL_UNWRAP_OR_RETURN(task.poll(fs, readable)) != nb::DeserializeResult::Ok) {
                    return etl::optional<WifiEvent>{};
                }
                return etl::optional<WifiEvent>{ReceiveDataFrame{task.result()}};
            }

            if (etl::holds_alternative<ReceiveControlFrameBody>(task_)) {
                auto &task = etl::get<ReceiveControlFrameBody>(task_);
                if (POLL_UNWRAP_OR_RETURN(task.poll(readable)) != nb::DeserializeResult::Ok) {
                    return etl::optional<WifiEvent>{};
                }
                return etl::optional{task.result()};
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
