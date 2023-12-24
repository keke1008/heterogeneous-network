#pragma once

#include <net/discovery.h>
#include <net/frame.h>
#include <net/neighbor.h>
#include <net/notification.h>

namespace net::routing {
    using SendError = neighbor::SendError;

    template <nb::AsyncReadableWritable RW>
    class RoutingService {
        template <nb::AsyncReadableWritable, uint8_t FRAME_ID_CACHE_SIZE>
        friend class RoutingSocket;

      public:
        inline void execute(
            frame::FrameService &fs,
            link::LinkService<RW> &ls,
            const local::LocalNodeService &lns,
            notification::NotificationService &notification_service,
            neighbor::NeighborService<RW> &ns,
            discovery::DiscoveryService<RW> &ds,
            util::Time &time,
            util::Rand &rand
        ) {}
    };
} // namespace net::routing
