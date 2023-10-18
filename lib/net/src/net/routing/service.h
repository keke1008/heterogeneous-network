#pragma once

#include "./link_state/service.h"
#include "./neighbor/service.h"
#include <net/frame/protocol_number.h>
#include <util/visitor.h>

namespace net::routing {
    class RoutingService {
        neighbor::NeighborService neighbor_service_;
        link_state::LinkStateService link_state_service_;
        neighbor::Event unhandled_neighbor_event_;

      public:
        explicit RoutingService(const NodeId &self_node_id, Cost self_node_cost)
            : link_state_service_{self_node_id, self_node_cost} {}

        inline NodeId self_id() const {
            return link_state_service_.self_id();
        }

        inline Cost self_cost() const {
            return link_state_service_.self_cost();
        }

        inline nb::Poll<void> request_send_hello(const link::Address &destination, Cost edge_cost) {
            return neighbor_service_.request_send_hello(
                destination, self_id(), self_cost(), edge_cost
            );
        }

        inline nb::Poll<void> request_send_goodbye(const link::Address &destination) {
            return neighbor_service_.request_send_goodbye(destination, self_id());
        }

        inline etl::optional<NodeId> resolve_next_node(const NodeId &destination) const {
            return link_state_service_.get_route(destination);
        }

        inline etl::optional<etl::span<const link::Address>> resolve_media(const NodeId &node_id
        ) const {
            return neighbor_service_.get_media(node_id);
        }

        template <frame::IFrameService FrameService>
        void
        execute(FrameService &frame_service, link::LinkService &link_service, util::Rand &rand) {
            if (etl::holds_alternative<etl::monostate>(unhandled_neighbor_event_)) {
                unhandled_neighbor_event_ = neighbor_service_.execute(frame_service, link_service);
                etl::visit(
                    util::Visitor{
                        [&](neighbor::NodeConnectedEvent &e) {
                            link_state_service_.add_node_and_edge(
                                e.id, e.node_cost, self_id(), e.edge_cost
                            );
                        },
                        [&](neighbor::NodeDisconnectedEvent &e) {
                            link_state_service_.remove_node(e.id);
                        },
                        [&](etl::monostate &) {},
                    },
                    unhandled_neighbor_event_
                );
            }

            etl::visit(
                util::Visitor{
                    [&](neighbor::NodeConnectedEvent &e) {
                        auto poll = link_state_service_.request_send_node_addition(
                            rand, e.id, self_id(), e.node_cost, e.edge_cost
                        );
                        if (poll.is_ready()) {
                            unhandled_neighbor_event_ = etl::monostate{};
                        }
                    },
                    [&](neighbor::NodeDisconnectedEvent &e) {
                        auto poll = link_state_service_.request_send_node_removal(rand, e.id);
                        if (poll.is_ready()) {
                            unhandled_neighbor_event_ = etl::monostate{};
                        }
                    },
                    [&](etl::monostate &) {},
                },
                unhandled_neighbor_event_
            );

            link_state_service_.execute(frame_service, link_service, neighbor_service_);
        }
    };
} // namespace net::routing
