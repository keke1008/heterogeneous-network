#pragma once

#include "../node.h"
#include "./constants.h"
#include <data/vec.h>
#include <nb/buf/splitter.h>
#include <net/link.h>

namespace net::routing {
    class NeighborNode {
        NodeId id_;
        data::Vec<link::Address, MAX_MEDIA_PER_NODE> addresses_;

      public:
        explicit NeighborNode(const NodeId &id) : id_{id} {}

        inline const NodeId &id() const {
            return id_;
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
        AlreadyConnected,
        Full,
    };

    enum class RemoveNodeResult : uint8_t {
        Disconnected,
        NotFound,
    };

    class NeighborList {
        data::Vec<NeighborNode, MAX_NEIGHBOR_NODE_COUNT> neighbors;

        inline etl::optional<uint8_t> find_neighbor_node(const NodeId &node_id) const {
            for (uint8_t i = 0; i < neighbors.size(); i++) {
                if (neighbors[i].id() == node_id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

      public:
        AddLinkResult add_neighbor_link(const NodeId &node_id, const link::Address &address) {
            if (neighbors.full()) {
                return AddLinkResult::Full;
            }

            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors[opt_index.value()];
                node.add_address_if_not_exists(address);
                return AddLinkResult::AlreadyConnected;
            }

            neighbors.emplace_back(node_id);
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
        void get_neighbors(data::Vec<NodeId, N> &dest) const {
            static_assert(N >= MAX_NEIGHBOR_NODE_COUNT, "N is too small");
            for (auto &node : this->neighbors) {
                if (dest.full()) {
                    break;
                }
                dest.push_back(node.id());
            }
        }
    };
} // namespace net::routing
