#pragma once

#include "./worker/accept.h"
#include "./worker/deserialize.h"
#include "./worker/discovery.h"
#include "./worker/send.h"
#include "./worker/send_broadcast.h"
#include "./worker/send_unicast.h"

namespace net::routing::worker {
    template <uint8_t FRAME_ID_CACHE_SIZE>
    class Worker {
        DeserializeWorker deserialize_;
        ReceiveWorker<FRAME_ID_CACHE_SIZE> receive_;
        SendWorker send_;
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
            receive_.execute(accept_, send_, lns);
            send_.execute(discovery_, send_broadcast_);
            send_broadcast_.execute(ns, neighbor_socket);
            discovery_.execute(send_unicast_, lns, ns, ds, time, rand);
            send_unicast_.execute(ns, neighbor_socket);
        }

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return accept_.poll_frame();
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame(
            const node::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            return send_.poll_send_frame(lns, destination, etl::move(reader));
        }

        inline frame::FrameId generate_frame_id() {
            return receive_.generate_frame_id();
        }
    };
} // namespace net::routing::worker
