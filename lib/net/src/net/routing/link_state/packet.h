#pragma once

#include "./table.h"

namespace net::routing::link_state {
    // フレームフォーマット
    //
    // 1. ノード数
    // 2. ノードのID (×ノード数)
    // 3. ノードのコスト (複数)
    class ExportTable {
        enum class State : uint8_t {
            NODE_COUNT,
            NODES,
            EDGES,
            DONE,
        } state_{State::NODE_COUNT};

        const Table &table_;
        uint8_t node_count_ = 0;
        NodeIndex i_ = 0;
        NodeIndex j_ = 0;

      public:
        explicit ExportTable(const Table &table) : table_{table} {
            for (NodeIndex i = 0; i < table_.max_node_count(); i++) {
                if (table_.resolve_node(i).has_value()) {
                    node_count_++;
                }
            }
        }

        nb::Poll<void> execute(net::stream::StreamWriter &writer) {
            if (state_ == State::NODE_COUNT) {
                if (writer.writable_count() < 1) {
                    writer.request_next_frame();
                    return nb::pending;
                }
                writer.write(node_count_);
                state_ = State::NODES;
            }

            if (state_ == State::NODES) {
                for (; i_ < table_.max_node_count(); i_++) {
                    const auto &node = table_.resolve_node(i_);
                    if (!node.has_value()) {
                        continue;
                    }

                    const auto &id = node->id;
                    const auto cost = node->cost;
                    uint8_t length = id.serialized_length() + cost.serialized_length();
                    if (writer.writable_count() >= length) {
                        writer.write(id, cost);
                    } else {
                        writer.request_next_frame();
                        return nb::pending;
                    }
                }

                state_ = State::EDGES;
                i_ = 0;
                j_ = i_ + 1;
            }

            if (state_ == State::EDGES) {
                while (i_ < table_.max_node_count()) {
                    auto node1 = table_.resolve_node(i_);
                    if (!node1.has_value()) {
                        i_++;
                        j_ = i_ + 1;
                        continue;
                    }

                    auto &peers = node1->peers;
                    for (; j_ < peers.size(); j_++) {
                        auto peer = peers.at(j_);

                        constexpr uint8_t length = 1 + 1 + peer.cost.serialized_length();
                        if (writer.writable_count() >= length) {
                            writer.write(i_);
                            writer.write(peer.id);
                            writer.write(peer.cost);
                        } else {
                            writer.request_next_frame();
                            return nb::pending;
                        }
                    }

                    i_++;
                    j_ = i_ + 1;
                }

                writer.close();
                state_ = State::DONE;
            }

            return nb::ready();
        }
    };

    // フレームフォーマット
    //
    // ExportTableと同じ
    class ImportTable {
        static constexpr uint8_t NODE_FULL = etl::numeric_limits<uint8_t>::max();

        enum class State : uint8_t {
            NODE_COUNT,
            NODES,
            EDGES,
            DONE,
        } state_{State::NODE_COUNT};

        Table &table_;
        etl::vector<NodeIndex, MAX_NODES> packet_index_to_table_index_;
        uint8_t node_count_;
        uint8_t i_;

        void push_node(const NodeId &node_id, const Cost cost) {
            auto table_index = table_.get_or_insert_node(node_id, cost);
            if (table_index.has_value()) {
                packet_index_to_table_index_.push_back(table_index.value());
            } else {
                packet_index_to_table_index_.push_back(NODE_FULL);
            }
        }

      public:
        explicit ImportTable(Table &table) : table_{table} {}

        nb::Poll<void> execute(net::stream::StreamReader &reader) {
            if (reader.is_buffer_filled()) {
                return nb::pending;
            }

            if (state_ == State::NODE_COUNT) {
                if (reader.readable_count() == 0) {
                    return nb::pending;
                }
                node_count_ = reader.read();
                i_ = 0;
                state_ = State::NODES;
            }

            if (state_ == State::NODES) {
                for (; i_ < node_count_; i_++) {
                    if (reader.readable_count() == 0) {
                        return nb::pending;
                    }

                    NodeId node_id = reader.read<NodeIdParser>();
                    Cost cost = reader.read<CostParser>();
                    push_node(node_id, cost);
                }

                state_ = State::EDGES;
            }

            if (state_ == State::EDGES) {
                while (!reader.is_closed()) {
                    if (reader.readable_count() == 0) {
                        return nb::pending;
                    }

                    // node1 < node2
                    NodeIndex packet_index1 = reader.read();
                    NodeIndex packet_index2 = reader.read();
                    NodeIndex table_index1 = packet_index_to_table_index_[packet_index1];
                    NodeIndex table_index2 = packet_index_to_table_index_[packet_index2];
                    if (table_index1 == NODE_FULL || table_index2 == NODE_FULL) {
                        continue;
                    }

                    Cost cost = reader.read<CostParser>();
                    table_.insert_edge(table_index1, table_index2, cost);
                }

                state_ = State::DONE;
            }

            return nb::ready();
        }
    };

    // フレームフォーマット
    //
    // 1. 追加されたノードのID
    // 2. 追加されたノードのコスト
    // 3. 追加されたノードの隣接ノードのID
    class AddNode {
        Table &table_;

      public:
        explicit AddNode(Table &table) : table_{table} {}

        nb::Poll<void> execute(net::stream::StreamReader &reader) {
            if (reader.is_buffer_filled()) {
                return nb::pending;
            }

            auto node_id = reader.read<NodeIdParser>();
            auto cost = reader.read<CostParser>();
            auto node_index = table_.get_or_insert_node(node_id, cost);
            if (!node_index.has_value()) {
                return nb::ready();
            }

            auto peer = reader.read<NodeIdParser>();
            auto peer_index = table_.get_node(peer);
            if (!peer_index.has_value()) {
                return nb::ready();
            }

            table_.insert_edge(node_index.value(), peer_index.value(), cost);
            return nb::ready();
        }
    };

    // フレームフォーマット
    //
    // 1. 削除されたノードのID
    class RemoveNode {
        Table &table_;

      public:
        explicit RemoveNode(Table &table) : table_{table} {}

        nb::Poll<void> execute(net::stream::StreamReader &reader) {
            if (reader.is_buffer_filled()) {
                return nb::pending;
            }

            auto node_id = reader.read<NodeIdParser>();
            auto node_index = table_.get_node(node_id);
            if (!node_index.has_value()) {
                return nb::ready();
            }

            table_.remove_node(node_index.value());
            return nb::ready();
        }
    };
} // namespace net::routing::link_state
