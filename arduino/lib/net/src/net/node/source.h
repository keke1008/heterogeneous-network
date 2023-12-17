#pragma once

#include "./cluster_id.h"
#include "./destination.h"
#include "./node_id.h"

namespace net::node {
    struct Source {
        NodeId node_id;
        OptionalClusterId cluster_id_;

      public:
        static etl::optional<Source> from_destination(const Destination &destination) {
            const auto &opt_node_id = destination.node_id;
            if (!opt_node_id) {
                return etl::nullopt;
            }
            const auto &opt_cluster_id = destination.cluster_id;
            return Source{
                .node_id = *opt_node_id,
                .cluster_id_ = opt_cluster_id ? OptionalClusterId{*opt_cluster_id}
                                              : OptionalClusterId::no_cluster(),
            };
        }

        explicit operator Destination() const {
            return cluster_id_.has_value() ? Destination{node_id, cluster_id_.value()}
                                           : Destination{node_id};
        }
    };

    class AsyncSourceDeserializer {
        AsyncNodeIdDeserializer node_id_;
        AsyncOptionalClusterIdDeserializer cluster_id_;

      public:
        inline Source result() const {
            return Source{
                .node_id = node_id_.result(),
                .cluster_id_ = cluster_id_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(node_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(cluster_id_.deserialize(r));
            return nb::DeserializeResult::Ok;
        }
    };

    class AsyncSourceSerializer {
        AsyncNodeIdSerializer node_id_;
        AsyncOptionalClusterIdSerializer cluster_id_;

      public:
        inline AsyncSourceSerializer(const Source &source)
            : node_id_{source.node_id},
              cluster_id_{source.cluster_id_} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(node_id_.serialize(w));
            return cluster_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return node_id_.serialized_length() + cluster_id_.serialized_length();
        }
    };
} // namespace net::node
