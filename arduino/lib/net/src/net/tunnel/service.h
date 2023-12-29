#pragma once

#include "./constants.h"
#include <net/routing.h>

namespace net::tunnel {
    template <nb::AsyncReadableWritable RW>
    class TunnelService {
        routing::RoutingSocket<RW, FRAME_DELAY_POOL_SIZE> socket_;

      public:
        explicit TunnelService(link::LinkService<RW> &ls)
            : socket_{ls.open(frame::ProtocolNumber::Tunnel)} {}

        inline void execute(
            frame::FrameService &fs,
            const local::LocalNodeService &lns,
            notification::NotificationService &nts,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(fs, lns, ns, ds, time, rand);

            // Arduinoではフレームを捨てる
            auto &&poll_frame = socket_.poll_receive_frame();
            if (poll_frame.is_ready()) {
                nts.notify(notification::FrameReceived{});
            }
        }
    };
}; // namespace net::tunnel
