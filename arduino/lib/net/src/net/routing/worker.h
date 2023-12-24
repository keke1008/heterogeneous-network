#pragma once

#include "./worker/accept.h"
#include "./worker/deserialize.h"
#include "./worker/discovery.h"
#include "./worker/link_delay.h"
#include "./worker/node_delay.h"
#include "./worker/send.h"
#include "./worker/send_broadcast.h"
#include "./worker/send_unicast.h"

namespace net::routing::worker {
    constexpr uint8_t DELAY_FRAME_CAPACITY = 2;

    template <uint8_t FRAME_ID_CACHE_SIZE>
    class Worker {
        DeserializeWorker deserialize_;
        LinkDelayWorker<DELAY_FRAME_CAPACITY> link_delay_;
        ReceiveWorker<FRAME_ID_CACHE_SIZE> receive_;
        NodeDelayWorker<DELAY_FRAME_CAPACITY> node_delay_;
        SendWorker send_;
        SendBroadcastWorker send_broadcast_;
        DiscoveryWorker discovery_;
        SendUnicastWorker send_unicast_;
        AcceptWorker accept_;

      public:
        template <nb::AsyncReadableWritable RW>
        void execute(
            const local::LocalNodeService &lns,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            neighbor::NeighborSocket<RW> &neighbor_socket,
            util::Time &time,
            util::Rand &rand
        ) {
            deserialize_.execute(link_delay_, ns, neighbor_socket, time);
            link_delay_.execute(receive_, lns, time);
            receive_.execute(accept_, node_delay_, lns, time);
            node_delay_.execute(send_, lns, time);
            send_.execute(discovery_, send_broadcast_);
            send_broadcast_.execute(ns, neighbor_socket);
            discovery_.execute(send_unicast_, lns, ns, ds, time, rand);
            send_unicast_.execute(ns, neighbor_socket);
        }

        inline nb::Poll<RoutingFrame> poll_receive_frame() {
            return accept_.poll_frame();
        }

        nb::Poll<nb::Future<etl::expected<void, neighbor::SendError>>> poll_send_frame(
            const local::LocalNodeService &lns,
            const node::Destination &destination,
            frame::FrameBufferReader &&reader
        ) {
            return send_.poll_send_frame(lns, destination, etl::move(reader));
        }

        inline frame::FrameId generate_frame_id(util::Rand &rand) {
            return receive_.generate_frame_id(rand);
        }
    };
} // namespace net::routing::worker
