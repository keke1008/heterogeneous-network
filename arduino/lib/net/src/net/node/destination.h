#pragma once

#include "./cluster_id.h"
#include "./node_id.h"
#include <etl/variant.h>

namespace net::node {
    struct Destination {
        friend class AsnycDestinationSerializer;

        node::NodeId node_id;
        node::OptionalClusterId cluster_id;

        static inline Destination broadcast() {
            return Destination{node::NodeId::broadcast(), node::OptionalClusterId::no_cluster()};
        }

        static inline Destination node(const node::NodeId &node_id) {
            return Destination{node_id, node::OptionalClusterId::no_cluster()};
        }

        static inline Destination cluster(const node::OptionalClusterId &cluster_id) {
            return Destination{node::NodeId::broadcast(), cluster_id};
        }

        static inline Destination
        node_and_cluster(const node::NodeId &node_id, node::OptionalClusterId cluster_id) {
            return Destination{node_id, cluster_id};
        }

        inline bool operator==(const Destination &other) const {
            return node_id == other.node_id && cluster_id == other.cluster_id;
        }

        inline bool operator!=(const Destination &other) const {
            return node_id != other.node_id || cluster_id != other.cluster_id;
        }

        inline bool is_broadcast() const {
            return node_id.is_broadcast() && cluster_id.is_no_cluster();
        }

        inline bool is_unicast() const {
            return !node_id.is_broadcast();
        }

        inline bool is_multicast() const {
            return node_id.is_broadcast() && cluster_id.has_value();
        }

        friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const Destination &destination) {
            return printer << '(' << destination.node_id << ',' << destination.cluster_id << ')';
        }
    };

    class AsyncDestinationDeserializer {
        node::AsyncNodeIdDeserializer node_id_;
        node::AsyncOptionalClusterIdDeserializer cluster_id_;

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

    class AsyncDestinationSerializer {
        node::AsyncNodeIdSerializer node_id_;
        node::AsyncOptionalClusterIdSerializer cluster_id_;

      public:
        inline AsyncDestinationSerializer(const Destination &destination)
            : node_id_{destination.node_id},
              cluster_id_{destination.cluster_id} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(node_id_.serialize(w));
            return cluster_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return node_id_.serialized_length() + cluster_id_.serialized_length();
        }
    };
}; // namespace net::node
