#pragma once

#include "../neighbor.h"
#include "./cache.h"
#include "./frame.h"
#include "./task.h"

namespace net::routing::reactive {
    class ReactiveService {
        TaskExecutor task_executor_;
        RouteCache route_cache_;

        etl::optional<NodeId> self_id_;
        etl::optional<Cost> self_cost_;

        void handle_received_frame(link::LinkFrame &&frame) {}

      public:
        explicit ReactiveService(neighbor::NeighborSocket &&neighbor_socket)
            : task_executor_{etl::move(neighbor_socket)} {}

        inline void set_self_id(const NodeId &id) {
            self_id_ = id;
        }

        inline void set_self_cost(const Cost &cost) {
            self_cost_ = cost;
        }

        void execute(
            frame::FrameService &frame_service,
            neighbor::NeighborService &neighbor_service,
            neighbor::Event &neighbor_event,
            util::Rand &rand
        ) {
            if (!self_id_ || !self_cost_) {
                return;
            }

            if (etl::holds_alternative<neighbor::NodeDisconnectedEvent>(neighbor_event)) {
                auto &event = etl::get<neighbor::NodeDisconnectedEvent>(neighbor_event);
                route_cache_.remove(event.id);
            }

            task_executor_.execute(
                frame_service, neighbor_service, route_cache_, rand, *self_id_, *self_cost_
            );
        }
    };
} // namespace net::routing::reactive
