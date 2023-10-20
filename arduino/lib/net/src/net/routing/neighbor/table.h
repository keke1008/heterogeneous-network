#pragma once

#include "../node.h"
#include <nb/buf/splitter.h>
#include <net/link.h>

namespace net::routing {
    constexpr uint8_t MAX_MEDIA_PER_NODE = 4;
    constexpr uint8_t MAX_NEIGHBOR_NODE_COUNT = 10;

    class NeighborNode {
        NodeId id_;
        etl::vector<link::Address, MAX_MEDIA_PER_NODE> media_;

      public:
        explicit NeighborNode(const NodeId &id) : id_{id} {}

        inline const NodeId &id() const {
            return id_;
        }

        inline etl::span<const link::Address> media() const {
            return etl::span{media_.data(), media_.end()};
        }

        bool has_media(const link::Address &address) const {
            for (uint8_t i = 0; i < media_.size(); i++) {
                if (media_[i] == address) {
                    return true;
                }
            }
            return false;
        }

        void add_media_if_not_exists(const link::Address &address) {
            if (!(media_.full() || has_media(address))) {
                media_.push_back(address);
            }
        }
    };

    enum class AddNodeResult : uint8_t {
        Connected,
        AlreadyConnected,
        Full,
    };

    enum class RemoveNodeResult : uint8_t {
        Disconnected,
        NotFound,
    };

    class NeighborList {
        etl::vector<NeighborNode, MAX_NEIGHBOR_NODE_COUNT> neighbors;

        inline etl::optional<uint8_t> find_neighbor_node(const NodeId &node_id) const {
            for (uint8_t i = 0; i < neighbors.size(); i++) {
                if (neighbors[i].id() == node_id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

      public:
        AddNodeResult add_neighbor_node(const NodeId &node_id, const link::Address &address) {
            if (neighbors.full()) {
                return AddNodeResult::Full;
            }

            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors[opt_index.value()];
                node.add_media_if_not_exists(address);
                return AddNodeResult::AlreadyConnected;
            }

            neighbors.emplace_back(node_id);
            neighbors.back().add_media_if_not_exists(address);
            return AddNodeResult::Connected;
        }

        RemoveNodeResult remove_neighbor_node(const NodeId &node_id) {
            for (uint8_t i = 0; i < neighbors.size(); i++) {
                if (neighbors[i].id() == node_id) {
                    neighbors[i] = neighbors.back();
                    neighbors.pop_back();
                    return RemoveNodeResult::Disconnected;
                }
            }
            return RemoveNodeResult::NotFound;
        }

        etl::optional<etl::span<const link::Address>> get_media_list(const NodeId &node_id) const {
            auto opt_index = find_neighbor_node(node_id);
            if (opt_index.has_value()) {
                auto &node = neighbors[opt_index.value()];
                return node.media();
            } else {
                return etl::nullopt;
            }
        }

        uint8_t get_neighbors(etl::span<const NodeId *> neighbors) const {
            uint8_t count = 0;
            for (auto &node : this->neighbors) {
                if (count >= neighbors.size()) {
                    break;
                }
                neighbors[count++] = &node.id();
            }
            return count;
        }
    };
} // namespace net::routing
