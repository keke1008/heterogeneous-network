#pragma once

#include "../node.h"
#include "./constants.h"
#include <nb/buf/splitter.h>
#include <net/link.h>
#include <tl/vec.h>

namespace net::routing::neighbor {
    class NeighborNode {
        NodeId id_;
        Cost link_cost_;
        tl::Vec<link::Address, MAX_MEDIA_PER_NODE> addresses_;

      public:
        explicit NeighborNode(const NodeId &id, Cost link_cost) : id_{id}, link_cost_{link_cost} {}

        inline const NodeId &id() const {
            return id_;
        }

        inline Cost link_cost() const {
            return link_cost_;
        }

        inline void set_link_cost(Cost cost) {
            link_cost_ = cost;
        }

        inline etl::span<const link::Address> addresses() const {
            return addresses_.as_span();
        }

        inline bool has_address(const link::Address &address) const {
            return etl::any_of(
                addresses_.begin(), addresses_.end(),
                [&](const link::Address &addr) { return addr == address; }
            );
        }

        inline void add_address_if_not_exists(const link::Address &address) {
            if (!(addresses_.full() || has_address(address))) {
                addresses_.push_back(address);
            }
        }

        inline bool has_addresses_type(link::AddressType type) const {
            return etl::any_of(
                addresses_.begin(), addresses_.end(),
                [&](const link::Address &addr) { return addr.type() == type; }
            );
        }
    };

    enum class AddLinkResult : uint8_t {
        NewNodeConnected,
        CostUpdated,
        AlreadyConnected,
        Full,
    };

    enum class RemoveNodeResult : uint8_t {
        Disconnected,
        NotFound,
    };

    class NeighborList {
        tl::Vec<NeighborNode, MAX_NEIGHBOR_NODE_COUNT> neighbors;

        inline etl::optional<uint8_t> find_neighbor_node(const NodeId &node_id) const {
            for (uint8_t i = 0; i < neighbors.size(); i++) {
                if (neighbors[i].id() == node_id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

      public:
        AddLinkResult
        add_neighbor_link(const NodeId &node_id, const link::Address &address, Cost cost) {
            if (neighbors.full()) {
                return AddLinkResult::Full;
            }

            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors[opt_index.value()];
                node.add_address_if_not_exists(address);

                if (node.link_cost() == cost) {
                    return AddLinkResult::AlreadyConnected;
                } else {
                    node.set_link_cost(cost);
                    return AddLinkResult::CostUpdated;
                }
            }

            neighbors.emplace_back(node_id, cost);
            neighbors.back().add_address_if_not_exists(address);
            return AddLinkResult::NewNodeConnected;
        }

        RemoveNodeResult remove_neighbor_node(const NodeId &node_id) {
            auto opt_index = find_neighbor_node(node_id);
            if (!opt_index.has_value()) {
                return RemoveNodeResult::NotFound;
            }

            auto index = opt_index.value();
            neighbors[index] = neighbors.back();
            neighbors.pop_back();
            return RemoveNodeResult::Disconnected;
        }

        inline etl::optional<Cost> get_link_cost(const NodeId &neighbor_id) const {
            auto index = find_neighbor_node(neighbor_id);
            return index ? etl::optional(neighbors[*index].link_cost()) : etl::nullopt;
        }

        etl::optional<etl::span<const link::Address>> get_addresses_of(const NodeId &node_id
        ) const {
            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors[opt_index.value()];
                return node.addresses();
            } else {
                return etl::nullopt;
            }
        }

        template <uint8_t N>
        void get_neighbors(tl::Vec<NeighborNode, N> &dest) const {
            static_assert(N >= MAX_NEIGHBOR_NODE_COUNT, "N is too small");
            for (auto &node : this->neighbors) {
                if (dest.full()) {
                    break;
                }
                dest.push_back(node);
            }
        }
    };
} // namespace net::routing::neighbor
