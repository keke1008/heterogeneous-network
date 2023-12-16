#pragma once

#include <etl/variant.h>
#include <net/node.h>

namespace net::discovery {
    struct Broadcast {
        inline bool operator==(const Broadcast &) const {
            return true;
        }

        inline bool operator!=(const Broadcast &) const {
            return false;
        }
    };

    class AsyncBroadcastDeserializer {
      public:
        inline Broadcast result() const {
            return Broadcast{};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &) {
            return nb::DeserializeResult::Ok;
        }
    };

    class AsyncBroadcastSerializer {
      public:
        explicit AsyncBroadcastSerializer(const Broadcast &) {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &) {
            return nb::SerializeResult::Ok;
        }

        constexpr inline uint8_t serialized_length() const {
            return 0;
        }
    };

    struct NodeIdAndClusterId {
        node::NodeId node_id;
        node::ClusterId cluster_id;

        inline bool operator==(const NodeIdAndClusterId &other) const {
            return node_id == other.node_id && cluster_id == other.cluster_id;
        }

        inline bool operator!=(const NodeIdAndClusterId &other) const {
            return node_id != other.node_id || cluster_id != other.cluster_id;
        }
    };

    class AsyncNodeIdAndClusterIdDeserializer {
        node::AsyncNodeIdDeserializer node_id_;
        node::AsyncClusterIdDeserializer cluster_id_;

      public:
        inline NodeIdAndClusterId result() const {
            return NodeIdAndClusterId{
                .node_id = node_id_.result(),
                .cluster_id = cluster_id_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(node_id_.deserialize(r));
            return cluster_id_.deserialize(r);
        }
    };

    class AsyncNodeIdAndClusterIdSerializer {
        node::AsyncNodeIdSerializer node_id_;
        node::AsyncClusterIdSerializer cluster_id_;

      public:
        inline AsyncNodeIdAndClusterIdSerializer(const NodeIdAndClusterId &id)
            : node_id_{id.node_id},
              cluster_id_{id.cluster_id} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(node_id_.serialize(w));
            return cluster_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return node_id_.serialized_length() + cluster_id_.serialized_length();
        }
    };

    namespace {
        struct Visitor {
            template <typename T>
            inline bool operator()(const T &lhs, const T &rhs) const {
                return lhs == rhs;
            }

            template <typename T, typename U>
            inline bool operator()(const T &, const U &) const {
                return false;
            }
        };
    } // namespace

    class Destination {
        friend class AsyncDestinationDeserializer;
        friend class AsyncDestinationSerializer;

        using Variant = etl::variant<Broadcast, node::NodeId, node::ClusterId, NodeIdAndClusterId>;
        Variant destination_;

      public:
        explicit Destination(const Variant &destination) : destination_{destination} {}

        explicit Destination(const node::NodeId &node_id) : destination_{node_id} {}

        explicit Destination(node::ClusterId cluster_id) : destination_{cluster_id} {}

        explicit Destination(const NodeIdAndClusterId &node_id_and_cluster_id)
            : destination_{node_id_and_cluster_id} {}

        explicit Destination(const node::NodeId &node_id, node::ClusterId cluster_id)
            : destination_{NodeIdAndClusterId{node_id, cluster_id}} {}

        inline bool operator==(const Destination &other) const {
            return etl::visit(Visitor{}, destination_, other.destination_);
        }

        inline bool operator!=(const Destination &other) const {
            return !(*this == other);
        }

        inline bool matches(const node::NodeId &node_id, node::ClusterId cluster_id) const {
            return etl::visit(
                util::Visitor{
                    [&](const Broadcast &) { return true; },
                    [&](const node::NodeId &id) { return id == node_id; },
                    [&](node::ClusterId id) { return id == cluster_id; },
                    [&](const NodeIdAndClusterId &id) {
                        return id.node_id == node_id && id.cluster_id == cluster_id;
                    }
                },
                destination_
            );
        }

        using NodeIdResult = etl::optional<etl::reference_wrapper<const node::NodeId>>;

        inline NodeIdResult node_id() const {
            return etl::visit<NodeIdResult>(
                util::Visitor{
                    [&](const node::NodeId &id) { return etl::cref(id); },
                    [&](const NodeIdAndClusterId &id) { return etl::cref(id.node_id); },
                    [&](const auto &) { return etl::nullopt; },
                },
                destination_
            );
        }

        using ClusterIdResult = etl::optional<etl::reference_wrapper<const node::ClusterId>>;

        inline ClusterIdResult cluster_id() const {
            return etl::visit<ClusterIdResult>(
                util::Visitor{
                    [&](node::ClusterId id) { return etl::cref(id); },
                    [&](const NodeIdAndClusterId &id) { return etl::cref(id.cluster_id); },
                    [&](const auto &) { return etl::nullopt; },
                },
                destination_
            );
        }
    };

    class AsyncDestinationDeserializer {
        nb::de::Variant<
            AsyncBroadcastDeserializer,
            node::AsyncNodeIdDeserializer,
            node::AsyncClusterIdDeserializer,
            AsyncNodeIdAndClusterIdDeserializer>
            destination_;

      public:
        inline Destination result() const {
            return Destination{destination_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            return destination_.deserialize(r);
        }
    };

    class AsyncDestinationSerializer {
        nb::ser::Variant<
            AsyncBroadcastSerializer,
            node::AsyncNodeIdSerializer,
            node::AsyncClusterIdSerializer,
            AsyncNodeIdAndClusterIdSerializer>
            destination_;

      public:
        inline AsyncDestinationSerializer(const Destination &destination)
            : destination_{destination.destination_} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            return destination_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return destination_.serialized_length();
        }
    };
}; // namespace net::discovery
