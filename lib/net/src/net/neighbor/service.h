#pragma once

#include "./table.h"
#include "./tasks.h"
#include <etl/algorithm.h>
#include <etl/vector.h>
#include <net/link.h>
#include <net/protocol_number.h>
#include <stdint.h>

namespace net::neighbor {
    class NeighborService {
        NeighborTable table_;
        Task task_;

        template <frame::IFrameService FrameService>
        nb::Poll<void> request_transmission(
            FrameService &frame_service,
            link::Address &destination,
            MessageType message_type
        ) {
            auto transmission = POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_transmission(
                protocol::ProtocolNumber::Neighbor, destination, NEIGHBOR_MESSAGE_SIZE
            ));
            transmission.write(static_cast<uint8_t>(message_type));
            return nb::ready();
        }

      public:
        NeighborService() = default;
        NeighborService(const NeighborService &) = delete;
        NeighborService(NeighborService &&) = default;
        NeighborService &operator=(const NeighborService &) = delete;
        NeighborService &operator=(NeighborService &&) = default;

        template <frame::IFrameService FrameService>
        inline nb::Poll<bool>
        request_solicitation(FrameService &frame_service, link::Address &destination) {
            if (table_.full()) {
                return nb::ready(false);
            }
            POLL_UNWRAP_OR_RETURN(
                request_transmission(frame_service, destination, MessageType::Solicitation)
            );
            return nb::ready(true);
        }

        template <frame::IFrameService FrameService>
        inline nb::Poll<void>
        request_advertisement(FrameService &frame_service, link::Address &destination) {
            return request_transmission(frame_service, destination, MessageType::Advertisement);
        }

        template <frame::IFrameService FrameService>
        inline nb::Poll<void>
        request_disconnect(FrameService &frame_service, link::Address &destination) {
            return request_transmission(frame_service, destination, MessageType::Disconnect);
        }

        template <frame::IFrameService FrameService>
        void execute(FrameService &frame_service) {
            task_.execute(frame_service, table_);
        }
    };
} // namespace net::neighbor
