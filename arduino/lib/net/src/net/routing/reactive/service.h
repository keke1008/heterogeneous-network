#pragma once

#include "../neighbor.h"
#include "./cache.h"
#include "./discovery.h"
#include "./frame.h"
#include "./task.h"

namespace net::routing::reactive {
    class ReactiveService {
        friend class RouteDiscoverTask;

        TaskExecutor task_executor_;
        RouteCache route_cache_;
        Discovery discovery_;

        void handle_received_frame(link::LinkFrame &&frame) {}

      public:
        explicit ReactiveService(link::LinkService &link_service, util::Time &time)
            : task_executor_{neighbor::NeighborSocket{
                  link_service.open(frame::ProtocolNumber::RoutingReactive)}},
              discovery_{time} {}

        void on_neighbor_event(const neighbor::Event &neighbor_event) {
            etl::visit(
                util::Visitor{
                    [&](const neighbor::NodeConnectedEvent &event) {
                        route_cache_.add(event.id, event.id);
                    },
                    [&](const neighbor::NodeDisconnectedEvent &event) {
                        route_cache_.remove(event.id);
                    },
                    [&](const etl::monostate &) {},
                },
                neighbor_event
            );
        }

        void execute(
            frame::FrameService &frame_service,
            neighbor::NeighborService &neighbor_service,
            const NodeId &self_id,
            Cost self_cost,
            util::Time &time,
            util::Rand &rand
        ) {
            auto opt_event = task_executor_.execute(
                frame_service, neighbor_service, route_cache_, rand, self_id, self_cost
            );
            if (opt_event) {
                const auto &event = opt_event.value();
                discovery_.on_route_found(event.remote_id, event.gateway_id, event.cost);
            }

            discovery_.execute(time, route_cache_);
        }
    };

    class RouteDiscoverTask {
        DiscoveryHandler handler_;

      public:
        explicit RouteDiscoverTask(const NodeId &target_id) : handler_{target_id} {}

        nb::Poll<etl::optional<NodeId>> execute(
            ReactiveService &reactive_service,
            const NodeId &self_id,
            Cost self_cost,
            util::Time &time,
            util::Rand &rand
        ) {
            return handler_.execute(
                reactive_service.discovery_, reactive_service.route_cache_,
                reactive_service.task_executor_, self_id, self_cost, time, rand
            );
        }
    };

} // namespace net::routing::reactive
