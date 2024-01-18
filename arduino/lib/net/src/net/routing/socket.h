#pragma once

#include "./frame.h"
#include "./task.h"
#include <net/frame.h>
#include <net/neighbor.h>

namespace net::routing {
    template <nb::AsyncReadableWritable RW, uint8_t FRAME_DELAY_POOL_SIZE>
    class RoutingSocket {
        neighbor::NeighborSocket<RW, FRAME_DELAY_POOL_SIZE> socket_;
        task::TaskExecutor<RW, FRAME_DELAY_POOL_SIZE> task_{};

      public:
        explicit RoutingSocket(
            link::LinkSocket<RW> &&link_socket,
            neighbor::NeighborSocketConfig config
        )
            : socket_{etl::move(link_socket), config} {}

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return task_.poll_receive_frame();
        }

        inline nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>>
        poll_send_frame(const node::Destination &destination, frame::FrameBufferReader &&reader) {
            return task_.poll_send_frame(destination, etl::move(reader));
        }

        inline uint8_t
        max_payload_length(const node::Source &source, const node::Destination &destination) const {
            return socket_.max_payload_length() -
                AsyncRoutingFrameHeaderSerializer::get_serialized_length(source, destination, 0);
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            util::Rand &rand,
            const node::Destination &destination,
            uint8_t payload_length
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            FASSERT(payload_length <= max_payload_length(info.source, destination));

            uint8_t length = AsyncRoutingFrameHeaderSerializer::get_serialized_length(
                info.source, destination, payload_length
            );
            auto &&writer = POLL_MOVE_UNWRAP_OR_RETURN(socket_.poll_frame_writer(fs, lns, length));
            AsyncRoutingFrameHeaderSerializer serializer{
                info.source,
                destination,
                task_.generate_frame_id(rand),
            };
            writer.serialize_all_at_once(serializer);
            return writer.subwriter();
        }

        inline RoutingSocketEvent execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(lns, ns, time);
            return task_.execute(fs, lns, ns, ds, socket_, time, rand);
        }
    };
} // namespace net::routing
