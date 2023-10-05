#pragma once

#include "../node.h"
#include "./resolver.h"
#include <etl/algorithm.h>
#include <nb/buf/builder.h>
#include <nb/buf/splitter.h>
#include <net/link.h>

namespace net::routing::link_state {
    constexpr uint8_t MAX_NODES = 20;
    constexpr uint8_t MAX_EDGES = 40;

    using NodeIndex = uint8_t;

    class LinkStateRoutingTable final : nb::buf::BufferWriter {
        resolver::AdjacencyCost<MAX_NODES> cost_;
        etl::array<etl::optional<NodeAddress>, MAX_NODES> cost_index_to_node_;
        NodeIndex self_index_;

      public:
        explicit LinkStateRoutingTable(const NodeAddress &self_address) {
            cost_index_to_node_[0] = self_address;
            cost_.set_vertex(0, Cost{0});
            self_index_ = 0;
        }

        etl::optional<NodeIndex> get_node(const NodeAddress &address) const {
            for (uint8_t i = 0; i < cost_index_to_node_.size(); i++) {
                if (cost_index_to_node_[i] == address) {
                    return i;
                }
            }
            return etl::nullopt;
        }

        etl::optional<NodeIndex> insert_node(const NodeAddress &address, Cost cost) {
            for (uint8_t i = 0; i < cost_index_to_node_.size(); i++) {
                if (!cost_index_to_node_[i].has_value()) {
                    cost_index_to_node_[i] = address;
                    cost_.set_vertex(i, cost);
                    return i;
                }
            }

            return etl::nullopt;
        }

        inline bool insert_edge(NodeIndex source, NodeIndex destination, Cost cost) {
            return cost_.set(source, destination, cost);
        }

        inline void remove_node(NodeIndex node_index) {
            cost_index_to_node_[node_index] = etl::nullopt;
            cost_.remove_vertex(node_index);
        }

        etl::optional<NodeAddress> resolve_gateway(const NodeAddress &dest) const {
            auto dest_node_index = get_node(dest);
            if (!dest_node_index.has_value()) {
                return etl::nullopt;
            }

            etl::array<bool, MAX_NODES> valid_nodes;
            for (uint8_t i = 0; i < valid_nodes.size(); i++) {
                valid_nodes[i] = cost_index_to_node_[i].has_value();
            }
            auto gateway_index = resolver::resolve_gateway_vertex<MAX_NODES>(
                cost_, valid_nodes, self_index_, dest_node_index.value()
            );
            if (!gateway_index.has_value()) {
                return etl::nullopt;
            }

            return cost_index_to_node_[gateway_index.value()];
        }

        bool merge_from_frame(nb::buf::BufferSplitter &splitter) {
            bool changed = false;
            static constexpr NodeIndex NODE_FULL = etl::numeric_limits<NodeIndex>::max();
            etl::array<NodeIndex, MAX_NODES> packet_to_table;

            uint8_t node_count = splitter.split_1byte();
            for (uint8_t i = 0; i < node_count; i++) {
                NodeAddress node_address = splitter.parse<NodeAddressParser>();
                Cost cost{splitter.split_1byte()};

                auto node_index = get_node(node_address);
                if (node_index.has_value()) {
                    packet_to_table[i] = node_index.value();
                    continue;
                }

                auto new_node_index = insert_node(node_address, cost);
                if (new_node_index.has_value()) {
                    packet_to_table[i] = new_node_index.value();
                    changed = true;
                } else {
                    packet_to_table[i] = NODE_FULL;
                }
            }

            uint8_t edge_count = splitter.split_1byte();
            for (uint8_t i = 0; i < edge_count; i++) {
                NodeIndex source = packet_to_table[splitter.split_1byte()];
                NodeIndex destination = packet_to_table[splitter.split_1byte()];
                Cost cost{splitter.split_1byte()};

                if (source != NODE_FULL && destination != NODE_FULL) {
                    changed |= insert_edge(source, destination, cost);
                }
            }

            return changed;
        }

        void write_to_builder(nb::buf::BufferBuilder &builder) override {
            uint8_t node_count = etl::count_if(
                cost_index_to_node_.begin(), cost_index_to_node_.end(),
                [](const auto &node) { return node.has_value(); }
            );
            builder.append(node_count);

            for (uint8_t i = 0; i < cost_index_to_node_.size(); i++) {
                if (!cost_index_to_node_[i].has_value()) {
                    continue;
                }

                builder.append(cost_index_to_node_[i].value());
                builder.append(cost_.get_vertex(i).value());
            }

            uint8_t edge_count = 0;
            for (uint8_t i = 0; i < cost_index_to_node_.size(); i++) {
                if (!cost_index_to_node_[i].has_value()) {
                    continue;
                }
                for (uint8_t j = i + 1; j < cost_index_to_node_.size(); j++) {
                    if (!cost_index_to_node_[j].has_value()) {
                        continue;
                    }

                    if (!cost_.get(i, j).is_infinite()) {
                        edge_count++;
                    }
                }
            }
            builder.append(edge_count);

            for (uint8_t i = 0; i < cost_index_to_node_.size(); i++) {
                if (!cost_index_to_node_[i].has_value()) {
                    continue;
                }
                for (uint8_t j = i + 1; j < cost_index_to_node_.size(); j++) {
                    if (!cost_index_to_node_[j].has_value()) {
                        continue;
                    }

                    Cost cost = cost_.get(i, j);
                    if (!cost.is_infinite()) {
                        builder.append(i);
                        builder.append(j);
                        builder.append(cost.value());
                    }
                }
            }
        }
    };
} // namespace net::routing::link_state
