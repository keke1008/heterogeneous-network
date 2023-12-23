#pragma once

#include "./frame.h"
#include "./worker.h"
#include <net/frame.h>
#include <net/neighbor.h>

namespace net::routing {
    template <nb::AsyncReadableWritable RW, uint8_t FRAME_ID_CACHE_SIZE>
    class RoutingSocket {
        neighbor::NeighborSocket<RW> neighbor_socket_;
        worker::Worker<FRAME_ID_CACHE_SIZE> worker_;

      public:
        explicit RoutingSocket(link::LinkSocket<RW> &&link_socket)
            : neighbor_socket_{etl::move(link_socket)} {}

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return worker_.poll_receive_frame();
        }

        inline nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame(
            const node::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            return worker_.poll_send_frame(lns, destination, etl::move(reader));
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &frame_service,
            const node::LocalNodeService &local_node_service,
            util::Rand &rand,
            const node::Destination &destination,
            uint8_t payload_length
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());
            uint8_t total_length = AsyncRoutingFrameHeaderSerializer::get_serialized_length(
                info.source, destination, payload_length
            );
            FASSERT(total_length <= neighbor_socket_.max_payload_length());

            auto &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(total_length));
            AsyncRoutingFrameHeaderSerializer serializer{
                info.source,
                destination,
                worker_.generate_frame_id(rand),
            };
            writer.serialize_all_at_once(serializer);
            return writer.subwriter();
        }

        void execute(
            const node::LocalNodeService &local_node_service,
            neighbor::NeighborService<RW> &neighbor_service,
            discovery::DiscoveryService<RW> &discovery_service,
            util::Time &time,
            util::Rand &rand
        ) {
            neighbor_socket_.execute();
            worker_.execute(
                local_node_service, neighbor_service, discovery_service, neighbor_socket_, time,
                rand
            );
        }
    };
} // namespace net::routing
