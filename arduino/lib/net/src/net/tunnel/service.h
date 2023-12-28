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
            neighbor::NeighborService<RW> &ns,
            util::Time &time,
            util::Rand &rand
        ) {
            socket_.execute(fs, lns, ns, time, rand);

            // Arduinoではフレームを捨てる
            socket_.poll_receive_frame();
        }
    };
}; // namespace net::tunnel
