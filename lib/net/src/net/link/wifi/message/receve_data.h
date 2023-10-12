#pragma once

#include "../../media.h"
#include "../address.h"
#include <nb/buf.h>
#include <net/frame/service.h>

namespace net::link::wifi {
    static constexpr uint8_t MAX_HEADER_SIZE = 23;

    struct ReceiveFrameHeader {
        frame::ProtocolNumber protocol_number;
        uint8_t length;
        IPv4Address remote_address;
        uint16_t remote_port;

        static ReceiveFrameHeader
        parse(etl::span<const uint8_t> span, frame::ProtocolNumber protocol_number) {
            DEBUG_ASSERT(span.size() <= MAX_HEADER_SIZE, "Invalid header size");
            nb::buf::BufferSplitter splitter{span};
            return ReceiveFrameHeader{
                .protocol_number = protocol_number,
                .length = serde::dec::deserialize<uint8_t>(splitter.split_sentinel(',')),
                .remote_address = splitter.parse<IPv4AddressWithTrailingCommaParser>(),
                .remote_port = serde::dec::deserialize<uint16_t>(splitter.split_remaining()),
            };
        }

        inline Frame
        make_frame(const Address &self_address, frame::FrameBufferReader &&reader) const {
            return Frame{
                .protocol_number = protocol_number,
                .source = self_address,
                .destination = Address{remote_address},
                .length = static_cast<uint8_t>(length - frame::PROTOCOL_SIZE),
                .reader = etl::move(reader),
            };
        }
    };

    class ReceiveHader {
        nb::stream::SentinelWritableBuffer<MAX_HEADER_SIZE> header_{':'};
        nb::stream::FixedWritableBuffer<1> protocol_;

      public:
        inline nb::Poll<ReceiveFrameHeader> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(header_.write_all_from(stream));
            POLL_UNWRAP_OR_RETURN(protocol_.write_all_from(stream));
            auto protocol_number =
                nb::buf::parse<frame::ProtocolNumberParser>(protocol_.written_bytes());
            return ReceiveFrameHeader::parse(header_.written_bytes(), protocol_number);
        }
    };

    class CreateFrameWriter {
        uint8_t length_;

      public:
        explicit CreateFrameWriter(uint8_t length) : length_{length} {}

        template <net::frame::IFrameService FrameService>
        inline nb::Poll<frame::FrameBufferWriter> poll(FrameService &frame_service) {
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

    class DiscardBody {
        nb::stream::DiscardingCountWritableBuffer discarder_;

      public:
        explicit DiscardBody(uint8_t length) : discarder_{length} {}

        inline nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            return discarder_.write_all_from(stream);
        }
    };

    class ReceiveDataMessageHandler {
        etl::variant<ReceiveHader, CreateFrameWriter, ReceiveBody, DiscardBody> task_{};
        Address self_address_;
        bool discard_requested_;
        etl::optional<ReceiveFrameHeader> header_;

      public:
        ReceiveDataMessageHandler() = delete;
        ReceiveDataMessageHandler(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler(ReceiveDataMessageHandler &&) = default;
        ReceiveDataMessageHandler &operator=(const ReceiveDataMessageHandler &) = delete;
        ReceiveDataMessageHandler &operator=(ReceiveDataMessageHandler &&) = default;

        explicit ReceiveDataMessageHandler(const Address &self_address, bool discard_requested)
            : self_address_{self_address},
              discard_requested_{discard_requested} {}

        template <net::frame::IFrameService FrameService>
        nb::Poll<etl::optional<Frame>>
        execute(FrameService &service, nb::stream::ReadableWritableStream &stream) {
            if (etl::holds_alternative<ReceiveHader>(task_)) {
                auto &task = etl::get<ReceiveHader>(task_);
                header_ = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(stream));
                uint8_t length = header_->length - frame::PROTOCOL_SIZE;
                if (discard_requested_) {
                    task_.emplace<DiscardBody>(length);
                } else {
                    task_.emplace<CreateFrameWriter>(length);
                }
            }

            if (etl::holds_alternative<CreateFrameWriter>(task_)) {
                auto &task = etl::get<CreateFrameWriter>(task_);
                auto writer = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(service));
                task_.emplace<ReceiveBody>(etl::move(writer));
            }

            if (etl::holds_alternative<ReceiveBody>(task_)) {
                auto &task = etl::get<ReceiveBody>(task_);
                auto reader = POLL_MOVE_UNWRAP_OR_RETURN(task.poll(stream));
                return etl::optional(header_->make_frame(self_address_, etl::move(reader)));
            }

            if (etl::holds_alternative<DiscardBody>(task_)) {
                auto &task = etl::get<DiscardBody>(task_);
                POLL_UNWRAP_OR_RETURN(task.poll(stream));
                return nb::ready(etl::optional<Frame>{});
            }

            return nb::pending;
        }
    };
} // namespace net::link::wifi
