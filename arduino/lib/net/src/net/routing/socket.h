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
        explicit RoutingSocket(link::LinkSocket<RW> &&link_socket)
            : socket_{etl::move(link_socket)} {}

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return task_.poll_receive_frame();
        }

        inline nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame(
            const local::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            return task_.poll_send_frame(lns, destination, etl::move(reader));
        }

        inline nb::Poll<frame::FrameBufferWriter> poll_frame_writer(
            frame::FrameService &frame_service,
            const local::LocalNodeService &lns,
            util::Rand &rand,
            const node::Destination &destination,
            uint8_t payload_length
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(lns.poll_info());
            uint8_t total_length = AsyncRoutingFrameHeaderSerializer::get_serialized_length(
                info.source, destination, payload_length
            );
            FASSERT(total_length <= POLL_UNWRAP_OR_RETURN(socket_.max_payload_length(lns)));

            auto &&writer =
                POLL_MOVE_UNWRAP_OR_RETURN(frame_service.request_frame_writer(total_length));
            AsyncRoutingFrameHeaderSerializer serializer{
                info.source,
                destination,
                task_.generate_frame_id(rand),
            };
            writer.serialize_all_at_once(serializer);
            return writer.subwriter();
        }

        void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(lns, ns, time);
            task_.execute(fs, lns, ns, ds, socket_, time, rand);
        }
    };
} // namespace net::routing
