#pragma once

#include "./frame.h"
#include "./reactive.h"
#include <net/frame/protocol_number.h>
#include <net/neighbor.h>
#include <net/notification.h>

namespace net::routing {
    using SendError = neighbor::SendError;

    template <nb::AsyncReadableWritable RW>
    class RoutingService {
        friend class RouteDiscoverTask;
        friend class RequestSendFrameTask;

        template <nb::AsyncReadableWritable>
        friend class RoutingSocket;

        neighbor::NeighborService<RW> neighbor_service_;
        reactive::ReactiveService<RW> reactive_service_;

        etl::optional<node::NodeId> self_id_;
        node::Cost self_cost_{0};

      public:
        explicit RoutingService(link::LinkService<RW> &link_service, util::Time &time)
            : neighbor_service_{link_service},
              reactive_service_{link_service, time} {}

        inline const etl::optional<node::NodeId> &self_id() const {
            return self_id_;
        }

        inline nb::Poll<etl::reference_wrapper<const node::NodeId>> poll_self_id() const {
            return self_id_.has_value() ? nb::ready(etl::cref(*self_id_)) : nb::pending;
        }

        inline nb::Poll<void>
        poll_send_hello(const link::Address &destination, node::Cost link_cost) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_send_hello(
                destination, *self_id_, link_cost, self_cost_
            );
        }

        inline nb::Poll<void> poll_send_goodbye(const node::NodeId &destination) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_goodbye(destination, *self_id_);
        }

        template <uint8_t N>
        inline void get_neighbors(tl::Vec<neighbor::NeighborNode, N> &neighbors) {
            neighbor_service_.get_neighbors(neighbors);
        }

        inline void execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            notification::NotificationService &notification_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (!self_id_) {
                const auto &opt_self_id = link_service.get_media_address();
                if (opt_self_id) {
                    self_id_ = node::NodeId(opt_self_id.value());
                    LOG_INFO("Local Id set.");
                } else {
                    return;
                }
            }

            const neighbor::Event &event =
                neighbor_service_.execute(frame_service, link_service, *self_id_, self_cost_);
            reactive_service_.on_neighbor_event(event);
            etl::visit(
                util::Visitor{
                    [&](const neighbor::NodeConnectedEvent &e) {
                        notification_service.notify(notification::NeighborUpdated{
                            .local_cost = self_cost_,
                            .neighbor_id = e.id,
                            .neighbor_cost = e.node_cost,
                            .link_cost = e.link_cost,
                        });
                    },
                    [&](const neighbor::NodeDisconnectedEvent &e) {
                        notification_service.notify(notification::NeighborRemoved{
                            .neighbor_id = e.id,
                        });
                    },
                    [&](const etl::monostate &) {},
                },
                event
            );

            reactive_service_.execute(
                frame_service, neighbor_service_, *self_id_, self_cost_, time, rand
            );
        }
    };
} // namespace net::routing
