#pragma once

#include "../node.h"
#include <etl/priority_queue.h>
#include <etl/stack.h>
#include <nb/buf.h>
#include <net/link.h>
#include <net/stream/socket.h>

namespace net::routing::link_state {
    constexpr uint8_t MAX_NODES = 20;
    constexpr uint8_t MAX_PEERS = 10;

    using NodeIndex = uint8_t;

    struct Peer {
        NodeIndex node_index;
        Cost cost;
    };

    class Peers {
        uint8_t count_ = 0;
        etl::array<Peer, MAX_PEERS> peers_;

      public:
        Peers() = default;

        inline bool empty() const {
            return count_ == 0;
        }

        inline uint8_t size() const {
            return count_;
        }

        inline Peer &at(uint8_t index) {
            return peers_[index];
        }

        inline bool contains(NodeIndex id) const {
            for (uint8_t i = 0; i < count_; i++) {
                if (peers_[i].node_index == id) {
                    return true;
                }
            }
            return false;
        }

        inline bool insert(NodeIndex id, Cost cost) {
            if (count_ == peers_.size()) {
                return false;
            }

            peers_[count_++] = Peer{id, cost};
            return true;
        }

        inline const Peer *begin() const {
            return peers_.begin();
        }

        inline Peer *begin() {
            return peers_.begin();
        }

        inline const Peer *end() const {
            return peers_.begin() + count_;
        }

        inline Peer *end() {
            return peers_.begin() + count_;
        }

        inline void remove(NodeIndex id) {
            for (uint8_t i = 0; i < count_; i++) {
                if (peers_[i].node_index == id) {
                    peers_[i] = peers_[--count_];
                    return;
                }
            }
        }
    };

    struct Node {
        NodeId id;
        Cost cost;
        Peers peers;

        inline Node(NodeId id, Cost cost) : id{id}, cost{cost} {}

        inline bool has_peer() const {
            return !peers.empty();
        }
    };

    class Table {
        static constexpr uint8_t SELF_INDEX = 0;
        etl::vector<etl::optional<Node>, MAX_NODES> nodes_;

        void remove_isolated_nodes() {
            etl::array<bool, MAX_NODES> found;
            found.fill(false);
            found[SELF_INDEX] = true;

            etl::stack<NodeIndex, MAX_NODES> stack;
            stack.push(SELF_INDEX);
            while (!stack.empty()) {
                auto node_index = stack.top();
                stack.pop();

                for (auto peer : nodes_[node_index].value().peers) {
                    if (!found[peer.node_index]) {
                        found[peer.node_index] = true;
                        stack.push(peer.node_index);
                    }
                }
            }

            for (uint8_t i = 0; i < nodes_.size(); i++) {
                if (!found[i]) {
                    nodes_[i] = etl::nullopt;
                }
            }
        }

        etl::optional<NodeIndex> insert_node(const NodeId &id, Cost cost) {
            for (uint8_t i = 0; i < nodes_.size(); i++) {
                if (!nodes_[i].has_value()) {
                    nodes_[i].emplace(id, cost);
                    return i;
                }
            }

            return etl::nullopt;
        }

      public:
        explicit Table(const NodeId &self_id, Cost self_cost) {
            nodes_.emplace_back(self_id, self_cost);
        }

        inline const NodeId &self_id() const {
            return nodes_[SELF_INDEX].value().id;
        }

        inline Cost self_cost() const {
            return nodes_[SELF_INDEX].value().cost;
        }

        constexpr inline uint8_t max_node_count() const {
            return MAX_NODES;
        }

        inline const etl::optional<Node> &resolve_node(NodeIndex index) const {
            return nodes_[index];
        }

        etl::optional<NodeIndex> get_node(const NodeId &id) const {
            for (uint8_t i = 0; i < nodes_.size(); i++) {
                if (nodes_[i].has_value() && nodes_[i].value().id == id) {
                    return i;
                }
            }
            return etl::nullopt;
        }

        etl::optional<NodeIndex> get_or_insert_node(const NodeId &id, Cost cost) {
            auto node_index = get_node(id);
            if (node_index.has_value()) {
                return node_index;
            }

            auto inserted_index = insert_node(id, cost);
            if (inserted_index.has_value()) {
                return inserted_index;
            }

            remove_isolated_nodes();
            return insert_node(id, cost);
        }

        bool insert_edge_if_not_exists(NodeIndex node1, NodeIndex node2, Cost cost) {
            auto n1 = nodes_[node1].value();
            auto n2 = nodes_[node2].value();

            if (!n1.peers.contains(node2) && n1.peers.insert(node2, cost)) {
                n2.peers.insert(node1, cost);
                return true;
            } else {
                return false;
            }
        }

        void remove_node(NodeIndex node_index) {
            auto &peers = nodes_[node_index].value().peers;
            for (auto peer : peers) {
                auto &peer_node = nodes_[peer.node_index].value();
                peer_node.peers.remove(node_index);
            }
            nodes_[node_index] = etl::nullopt;
        }

        struct Route {
            uint16_t cost;
            NodeIndex gateway;
        };

        etl::optional<Route> resolve_route(NodeIndex destination) const {
            struct Vertex {
                uint16_t cost;
                NodeIndex node;
                NodeIndex gateway;

                inline constexpr bool operator<(const Vertex &other) const {
                    return cost < other.cost;
                }
            };

            etl::priority_queue<
                Vertex, MAX_NODES, etl::vector<Vertex, MAX_NODES>, etl::greater<Vertex>>
                queue;
            queue.push({
                .cost = 0,
                .node = SELF_INDEX,
                .gateway = SELF_INDEX,
            });

            etl::array<bool, MAX_NODES> visited;
            visited.fill(false);
            visited[SELF_INDEX] = true;
            etl::array<uint16_t, MAX_NODES> min_cost;
            min_cost.fill(etl::numeric_limits<uint16_t>::max());
            min_cost[SELF_INDEX] = 0;

            while (!queue.empty()) {
                Vertex v = queue.top();
                queue.pop();

                if (visited[v.node] && v.cost > min_cost[v.node]) {
                    continue;
                }
                visited[v.node] = true;

                if (v.node == destination) {
                    return Route{.cost = v.cost, .gateway = v.gateway};
                }

                auto &node = nodes_[v.node].value();
                for (const auto &peer : node.peers) {
                    if (visited[peer.node_index]) {
                        continue;
                    }

                    uint16_t new_cost = v.cost + node.cost.value() + peer.cost.value();
                    if (new_cost < min_cost[peer.node_index]) {
                        min_cost[peer.node_index] = new_cost;
                        queue.push({
                            .cost = new_cost,
                            .node = peer.node_index,
                            .gateway = v.gateway == SELF_INDEX ? peer.node_index : v.gateway,
                        });
                    }
                }
            }

            return etl::nullopt;
        }
    };
} // namespace net::routing::link_state
