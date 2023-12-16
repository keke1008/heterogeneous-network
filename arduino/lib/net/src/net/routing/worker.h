#pragma once

#include "./worker/accept.h"
#include "./worker/deserialize.h"
#include "./worker/discovery.h"
#include "./worker/send_broadcast.h"
#include "./worker/send_unicast.h"

namespace net::routing::worker {
    template <uint8_t FRAME_ID_CACHE_SIZE>
    class Worker {
        DeserializeWorker deserialize_;
        ReceiveWorker<FRAME_ID_CACHE_SIZE> receive_;
        SendBroadcastWorker send_broadcast_;
        DiscoveryWorker discovery_;
        SendUnicastWorker send_unicast_;
        AcceptWorker accept_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            const node::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            neighbor::NeighborSocket<RW> &neighbor_socket,
            util::Time &time,
            util::Rand &rand
        ) {
            deserialize_.execute(receive_, lns, neighbor_socket);
            receive_.execute(accept_, discovery_, send_broadcast_);
            send_broadcast_.execute(ns, neighbor_socket);
            discovery_.execute(send_unicast_, lns, ns, ds, time, rand);
            send_unicast_.execute(ns, neighbor_socket);
        }

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return accept_.poll_frame();
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>>
        poll_send_frame(const node::NodeId &destination_id, frame::FrameBufferReader &&reader) {
            return send_unicast_.poll_send_frame_with_result(destination_id, etl::move(reader));
        }

        inline nb::Poll<void> poll_send_broadcast_frame(
            frame::FrameBufferReader &&reader,
            const etl::optional<node::NodeId> &ignore_id = etl::nullopt
        ) {
            return send_broadcast_.poll_send_broadcast_frame(etl::move(reader), ignore_id);
        }

        inline frame::FrameId generate_frame_id() {
            return receive_.generate_frame_id();
        }
    };
} // namespace net::routing::worker
