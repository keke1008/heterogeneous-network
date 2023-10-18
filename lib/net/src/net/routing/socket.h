#pragma once

#include "./node.h"
#include "./service.h"
#include <debug_assert.h>

namespace net::routing {
    struct Frame {
        NodeId source_id;
        NodeId destination_id;
        frame::FrameBufferReader reader;
    };

    template <frame::IFrameService FrameService>
    class Socket {
        FrameService &frame_service_;
        link::LinkService &link_service_;
        RoutingService &routing_service_;

        etl::optional<Frame> repeating_frame_;

        frame::ProtocolNumber protocol_number_;
        NodeId source_id_;
        NodeId destination_id_;

        inline void write_header(frame::FrameBufferWriter &writer) {
            writer.write(source_id_, destination_id_);
        }

      public:
        Socket(const Socket &) = delete;
        Socket &operator=(const Socket &) = delete;
        Socket(Socket &&) = default;
        Socket &operator=(Socket &&) = delete;

        explicit Socket(
            FrameService &frame_service,
            link::LinkService &link_service,
            RoutingService &routing_service,
            frame::ProtocolNumber protocol_number,
            const NodeId &peer_id
        )
            : frame_service_{frame_service},
              link_service_{link_service},
              routing_service_{routing_service},
              protocol_number_{protocol_number},
              source_id_{routing_service.self_id()},
              destination_id_{peer_id} {}

        inline uint8_t max_frame_length() const {
            return frame::MTU - source_id_.serialized_length() -
                destination_id_.serialized_length();
        }

        inline nb::Poll<frame::FrameBufferWriter> request_frame_writer(uint8_t length) {
            DEBUG_ASSERT(length <= max_frame_length());
            uint8_t total_length =
                length + source_id_.serialized_length() + destination_id_.serialized_length();
            auto frame = POLL_MOVE_UNWRAP_OR_RETURN(frame_service_.request_frame_writer(length));
            write_header(frame);
            return etl::move(frame);
        }

        inline nb::Poll<frame::FrameBufferWriter> request_max_length_frame_writer() {
            auto frame =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service_.request_max_length_frame_writer());
            write_header(frame);
            return etl::move(frame);
        }

        enum class SendResult : uint8_t {
            Success,
            Unreachable,
        };

        nb::Poll<SendResult> send_frame(frame::FrameBufferReader &&frame) {
            auto address = routing_service_.resolve_gateway_address(destination_id_);
            if (!address.has_value()) {
                return nb::ready(SendResult::Unreachable);
            }

            POLL_UNWRAP_OR_RETURN(
                link_service_.send_frame(protocol_number_, address.value(), etl::move(frame))
            );
            return nb::ready(SendResult::Success);
        }

        nb::Poll<Frame> receive_frame() {
            if (repeating_frame_.has_value()) {
                return nb::pending;
            }

            auto frame = POLL_MOVE_UNWRAP_OR_RETURN(link_service_.receive_frame(protocol_number_));
            Frame received_frame{
                .source_id = frame.reader.read<NodeIdParser>(),
                .destination_id = frame.reader.read<NodeIdParser>(),
                .reader = etl::move(frame.reader),
            };
            if (received_frame.destination_id == source_id_) {
                return etl::move(received_frame);
            } else {
                repeating_frame_ = etl::move(received_frame);
                return nb::pending;
            }
        }

        inline void execute() {
            if (!repeating_frame_.has_value()) {
                return;
            }

            auto &frame = repeating_frame_.value();
            auto gateway_address = routing_service_.resolve_gateway_address(frame.source_id);
            if (!gateway_address.has_value()) {
                repeating_frame_ = etl::nullopt;
                return;
            }

            auto poll = link_service_.send_frame(
                protocol_number_, gateway_address.value(), etl::move(frame.reader)
            );
            if (poll.is_ready()) {
                repeating_frame_ = etl::nullopt;
            }
        }
    };
} // namespace net::routing
