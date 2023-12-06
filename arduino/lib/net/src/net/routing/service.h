#pragma once

#include "./reactive.h"
#include <net/frame.h>
#include <net/neighbor.h>
#include <net/notification.h>

namespace net::routing {
    using SendError = neighbor::SendError;

    template <nb::AsyncReadableWritable RW>
    class RoutingService {
        template <nb::AsyncReadableWritable, uint8_t FRAME_ID_CACHE_SIZE>
        friend class RoutingSocket;

        reactive::ReactiveService<RW> reactive_service_;

      public:
        explicit RoutingService(link::LinkService<RW> &link_service, util::Time &time)
            : reactive_service_{link_service, time} {}

        inline void execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            const node::LocalNodeService &local_node_service,
            notification::NotificationService &notification_service,
            neighbor::NeighborService<RW> &neighbor_service,
            util::Time &time,
            util::Rand &rand
        ) {
            const auto &poll_info = local_node_service.poll_info();
            if (poll_info.is_pending()) {
                return;
            }

            const auto &info = poll_info.unwrap();

            reactive_service_.execute(
                frame_service, local_node_service, neighbor_service, time, rand
            );
        }
    };
} // namespace net::routing
