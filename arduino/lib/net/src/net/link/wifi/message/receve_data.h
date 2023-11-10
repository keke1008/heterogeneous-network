#pragma once

#include "../../media.h"
#include "../frame.h"
#include <logger.h>
#include <nb/buf.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    static constexpr uint8_t MAX_HEADER_SIZE = 23;

    struct ReceivedFrameHeader {
        frame::ProtocolNumber protocol_number;
        uint8_t length;
        WifiAddress remote;

        static ReceivedFrameHeader
        parse(etl::span<const uint8_t> span, frame::ProtocolNumber protocol_number) {
            ASSERT(span.size() <= MAX_HEADER_SIZE);
            nb::buf::BufferSplitter splitter{span};
            return ReceivedFrameHeader{
                .protocol_number = protocol_number,
                .length = serde::dec::deserialize<uint8_t>(splitter.split_sentinel(',')),
                .remote = splitter.parse(WifiAddressDeserializer{','}),
            };
        }

        inline WifiFrame make_frame(frame::FrameBufferReader &&reader) const {
            return WifiFrame{
                .protocol_number = protocol_number,
                .remote = remote,
                .reader = etl::move(reader),
            };
        }
    };

    class ReceiveHader {
        nb::stream::SentinelWritableBuffer<MAX_HEADER_SIZE> header_{':'};
        nb::stream::FixedWritableBuffer<1> protocol_;

      public:
        inline nb::Poll<ReceivedFrameHeader> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(protocol_.write_all_from(stream));
            auto protocol_number =
                nb::buf::parse<frame::ProtocolNumberParser>(protocol_.written_bytes());
            return ReceivedFrameHeader::parse(header_.written_bytes(), protocol_number);
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

        inline nb::Poll<frame::FrameBufferReader> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(writer_.write_all_from(stream));
            return writer_.make_initial_reader();
        }
    };

    class ReceiveDataMessageHandler {
        etl::variant<ReceiveHader, CreateFrameWriter, ReceiveBody> task_{};
        etl::optional<ReceivedFrameHeader> header_;

      public:
        ReceiveDataMessageHandler() = default;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        nb::Poll<WifiFrame>
        execute(frame::FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<ReceiveHader>(task_)) {
                auto &task = etl::get<ReceiveHader>(task_);
                header_ = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(stream));
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
                auto reader = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(stream));
                return header_->make_frame(etl::move(reader));
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
