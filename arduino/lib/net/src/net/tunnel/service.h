#pragma once

#include "./constants.h"
#include <net/routing.h>

namespace net::tunnel {
    class TunnelService {
        routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> socket_;

      public:
        explicit TunnelService(link::LinkService &ls)
            : socket_{ls.open(frame::ProtocolNumber::Tunnel)} {}

        inline void execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            neighbor::NeighborService &ns,
            discovery::DiscoveryService &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            auto event = socket_.execute(fs, ms, lns, ns, ds, time, rand);
            if (event.frame_received) {
                nts.notify(notification::FrameReceived{});
            }

            // Arduinoでは受信できないのでフレームを捨てる
            socket_.poll_receive_frame();
        }
    };
}; // namespace net::tunnel
