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

        inline nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>>
        poll_send_frame(const node::NodeId &remote_id, frame::FrameBufferReader &&reader) {
            return worker_.poll_send_frame(remote_id, etl::move(reader));
        }

        inline nb::Poll<void> poll_send_broadcast_frame(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id = etl::nullopt
        ) {
            return worker_.poll_send_broadcast_frame(etl::move(reader), ignore_id);
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &frame_service,
            const node::LocalNodeService &local_node_service,
            const discovery::Destination &destination,
            uint8_t payload_length
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());
            uint8_t total_length = AsyncRoutingFrameSerializer::get_serialized_length(
                info.id, destination, payload_length
            );
            ASSERT(total_length <= neighbor_socket_.max_payload_length());

            auto &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(total_length));
            AsyncRoutingFrameSerializer serializer{
                RoutingFrame{
                    .source_id = info.id,
                    .destination = destination,
                    .frame_id = worker_.generate_frame_id(),
                    .payload = writer
                },
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
