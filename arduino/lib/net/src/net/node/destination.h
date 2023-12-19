#pragma once

#include "./cluster_id.h"
#include "./node_id.h"
#include <etl/variant.h>

namespace net::node {
    struct Destination {
        friend class AsnycDestinationSerializer;

        etl::optional<node::NodeId> node_id;
        etl::optional<node::ClusterId> cluster_id;

        static inline Destination broadcast() {
            return Destination{etl::nullopt, etl::nullopt};
        }

        static inline Destination node(const node::NodeId &node_id) {
            return Destination{node_id, etl::nullopt};
        }

        static inline Destination cluster(const node::ClusterId &cluster_id) {
            return Destination{etl::nullopt, cluster_id};
        }

        static inline Destination
        node_and_cluster(const node::NodeId &node_id, node::ClusterId cluster_id) {
            return Destination{node_id, cluster_id};
        }

        inline bool operator==(const Destination &other) const {
            return node_id == other.node_id && cluster_id == other.cluster_id;
        }

        inline bool operator!=(const Destination &other) const {
            return node_id != other.node_id || cluster_id != other.cluster_id;
        }

        inline bool is_broadcast() const {
            return !node_id && !cluster_id;
        }

        inline bool is_unicast() const {
            return node_id.has_value();
        }

        inline bool is_multicast() const {
            return !node_id && cluster_id;
        }
    };

    class AsyncBroadcastDeserializer {
      public:
        inline Destination result() const {
            return Destination::broadcast();
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &) {
            return nb::DeserializeResult::Ok;
        }
    };

    class AsyncNodeIdAndClusterIdDeserializer {
        node::AsyncNodeIdDeserializer node_id_;
        node::AsyncClusterIdDeserializer cluster_id_;

      public:
        inline Destination result() const {
            return Destination{
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

    class AsyncDestinationDeserializer {
        nb::de::Variant<
            AsyncBroadcastDeserializer,
            node::AsyncNodeIdDeserializer,
            node::AsyncClusterIdDeserializer,
            AsyncNodeIdAndClusterIdDeserializer>
            destination_;

      public:
        inline Destination result() const {
            return etl::visit(
                util::Visitor{
                    [](const Destination &destination) { return destination; },
                    [](const node::NodeId &id) { return Destination::node(id); },
                    [](node::ClusterId id) { return Destination::cluster(id); },
                },
                destination_.result()
            );
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            return destination_.deserialize(r);
        }
    };

    class AsyncBroadcastSerializer {
      public:
        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &) {
            return nb::SerializeResult::Ok;
        }

        constexpr inline uint8_t serialized_length() const {
            return 0;
        }
    };

    class AsyncNodeIdAndClusterIdSerializer {
        node::AsyncNodeIdSerializer node_id_;
        node::AsyncClusterIdSerializer cluster_id_;

      public:
        inline AsyncNodeIdAndClusterIdSerializer(const Destination &destination)
            : node_id_{*destination.node_id},
              cluster_id_{*destination.cluster_id} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(node_id_.serialize(w));
            return cluster_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return node_id_.serialized_length() + cluster_id_.serialized_length();
        }
    };

    class AsyncDestinationSerializer {
        nb::ser::Variant<
            AsyncBroadcastSerializer,
            node::AsyncNodeIdSerializer,
            node::AsyncClusterIdSerializer,
            AsyncNodeIdAndClusterIdSerializer>
            destination_;

        static etl::variant<
            AsyncBroadcastSerializer,
            node::AsyncNodeIdSerializer,
            node::AsyncClusterIdSerializer,
            AsyncNodeIdAndClusterIdSerializer>
        make_variant(const Destination &destination) {
            if (destination.node_id) {
                if (destination.cluster_id) {
                    return AsyncNodeIdAndClusterIdSerializer{destination};
                } else {
                    return node::AsyncNodeIdSerializer{*destination.node_id};
                }
            } else {
                if (destination.cluster_id) {
                    return node::AsyncClusterIdSerializer{*destination.cluster_id};
                } else {
                    return AsyncBroadcastSerializer{};
                }
            }
        }

      public:
        inline AsyncDestinationSerializer(const Destination &destination)
            : destination_{make_variant(destination)} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            return destination_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return destination_.serialized_length();
        }
    };
}; // namespace net::node
