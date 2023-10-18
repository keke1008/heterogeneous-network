#pragma once

#include "./cache.h"
#include "./link_state/service.h"
#include "./neighbor/service.h"
#include <net/frame/protocol_number.h>
#include <util/visitor.h>

namespace net::routing {
    using link_state::NextNode;

    class RoutingService {
        neighbor::NeighborService neighbor_service_;
        link_state::LinkStateService link_state_service_;
        GatewayCache gateway_cache_;
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

        inline nb::Poll<void> request_hello(const link::Address &destination, Cost edge_cost) {
            return neighbor_service_.request_hello(destination, self_id(), self_cost(), edge_cost);
        }

        inline nb::Poll<void> request_goodbye(const NodeId &destination) {
            return neighbor_service_.request_goodbye(destination, self_id());
        }

        inline etl::optional<NextNode> resolve_next_node(const NodeId &destination) const {
            return link_state_service_.get_route(destination);
        }

        inline etl::optional<link::Address> resolve_gateway_address(const NodeId &destination) {
            auto cached_gateway_address = gateway_cache_.get(destination);
            if (cached_gateway_address.has_value()) {
                return cached_gateway_address;
            }

            auto next_node = resolve_next_node(destination);
            if (next_node.has_value()) {
                auto media_list = neighbor_service_.get_media_list(next_node->node_id);
                if (media_list.has_value()) {
                    gateway_cache_.add(destination, media_list->front());
                    return media_list->front();
                }
            }
            return etl::nullopt;
        }

        inline etl::optional<etl::span<const link::Address>> get_media_list(const NodeId &node_id
        ) const {
            return neighbor_service_.get_media_list(node_id);
        }

        template <frame::IFrameService FrameService>
        void
        execute(FrameService &frame_service, link::LinkService &link_service, util::Rand &rand) {
            if (etl::holds_alternative<etl::monostate>(unhandled_neighbor_event_)) {
                unhandled_neighbor_event_ =
                    neighbor_service_.execute(frame_service, link_service, self_id(), self_cost());
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
