#pragma once

#include "./neighbor.h"
#include "./reactive.h"
#include <net/frame/protocol_number.h>

namespace net::routing {
    class RoutingService {
        friend class RouteDiscoverTask;

        neighbor::NeighborService neighbor_service_;
        reactive::ReactiveService reactive_service_;

        etl::optional<NodeId> self_id_;
        Cost self_cost_{0};

      public:
        explicit RoutingService(link::LinkService &link_service, util::Time &time)
            : neighbor_service_{link_service},
              reactive_service_{link_service, time} {}

        inline const etl::optional<NodeId> &self_id() const {
            return self_id_;
        }

        inline void set_self_id(const NodeId &id) {
            self_id_ = id;
        }

        inline nb::Poll<void> poll_send_hello(const link::Address &destination) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_send_hello(destination, *self_id_, self_cost_);
        }

        inline nb::Poll<void> poll_send_goodbye(const NodeId &destination) {
            if (!self_id_) {
                return nb::pending;
            }
            return neighbor_service_.request_goodbye(destination, *self_id_);
        }

        template <uint8_t N>
        inline void get_neighbors(data::Vec<neighbor::NeighborNode, N> &neighbors) {
            neighbor_service_.get_neighbors(neighbors);
        }

        inline void execute(
            frame::FrameService &frame_service,
            link::LinkService &link_service,
            util::Rand &rand
        ) {
            if (!self_id_) {
                return;
            }

            const auto &event = neighbor_service_.execute(frame_service, link_service, *self_id_);
            reactive_service_.on_neighbor_event(event);

            reactive_service_.execute(
                frame_service, neighbor_service_, *self_id_, self_cost_, rand
            );
        }
    };

    class RouteDiscoverTask {
        reactive::RouteDiscoverTask inner_task_;

      public:
        explicit RouteDiscoverTask(const NodeId &target_id) : inner_task_{target_id} {}

        inline nb::Poll<etl::optional<NodeId>>
        execute(RoutingService &routing_service, util::Time &time, util::Rand &rand) {
            if (!routing_service.self_id_) {
                return nb::pending;
            }

            return inner_task_.execute(
                routing_service.reactive_service_, *routing_service.self_id_,
                routing_service.self_cost_, time, rand
            );
        }
    };

} // namespace net::routing
