#pragma once

#include "./reactive.h"
#include <net/frame.h>
#include <net/neighbor.h>
#include <net/notification.h>

namespace net::routing {
    using SendError = neighbor::SendError;

    template <nb::AsyncReadableWritable RW>
    class RoutingService {
        friend class RouteDiscoverTask;
        friend class RequestSendFrameTask;

        template <nb::AsyncReadableWritable, uint8_t FRAME_ID_CACHE_SIZE>
        friend class RoutingSocket;

        neighbor::NeighborService<RW> neighbor_service_;
        reactive::ReactiveService<RW> reactive_service_;

      public:
        explicit RoutingService(link::LinkService<RW> &link_service, util::Time &time)
            : neighbor_service_{link_service},
              reactive_service_{link_service, time} {}

        inline nb::Poll<void> poll_send_hello(
            const node::LocalNodeService &local_node_service,
            const link::Address &destination,
            node::Cost link_cost
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());
            return neighbor_service_.request_send_hello(destination, info.id, link_cost, info.cost);
        }

        inline nb::Poll<void> poll_send_goodbye(
            const node::LocalNodeService &local_node_service,
            const node::NodeId &destination
        ) {
            const auto &info = POLL_UNWRAP_OR_RETURN(local_node_service.poll_info());
            return neighbor_service_.request_goodbye(destination, info.id);
        }

        template <uint8_t N>
        inline void get_neighbors(tl::Vec<neighbor::NeighborNode, N> &neighbors) {
            neighbor_service_.get_neighbors(neighbors);
        }

        inline void execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            const node::LocalNodeService &local_node_service,
            notification::NotificationService &notification_service,
            util::Time &time,
            util::Rand &rand
        ) {
            const auto &poll_info = local_node_service.poll_info();
            if (poll_info.is_pending()) {
                return;
            }

            const auto &info = poll_info.unwrap();

            neighbor_service_.execute(
                frame_service, link_service, local_node_service, notification_service
            );

            reactive_service_.execute(
                frame_service, local_node_service, neighbor_service_, time, rand
            );
        }
    };
} // namespace net::routing
