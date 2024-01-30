#pragma once

#include <nb/serde.h>
#include <net/notification.h>
#include <stdint.h>

namespace net::observer {
    enum class FrameType : uint8_t {
        NodeSubscription = 1,
        NodeNotification = 2,
    };

    static constexpr bool is_valid_frame_type(uint8_t frame_type) {
        return frame_type == static_cast<uint8_t>(FrameType::NodeSubscription) ||
            frame_type == static_cast<uint8_t>(FrameType::NodeNotification);
    }

    using AsyncFrameTypeDeserializer = nb::de::Enum<FrameType, is_valid_frame_type>;

    using AsyncEntryCountSerializer = nb::ser::Bin<uint8_t>;

    enum class NodeNotificationEntryType : uint8_t {
        SelfUpdated = 1,
        NeighborUpdated = 2,
        NeighborRemoved = 3,
        FrameReceived = 4,
    };

    using AsyncNodeNotificationEntryTypeSerializer = nb::ser::Enum<NodeNotificationEntryType>;

    class AsyncSelfUpdateEntrySerializer {
        node::AsyncOptionalClusterIdSerializer cluster_id_;
        node::AsyncCostSerializer cost_;

      public:
        explicit AsyncSelfUpdateEntrySerializer(const notification::SelfUpdated &self_updated)
            : cluster_id_{etl::move(self_updated.cluster_id)},
              cost_{etl::move(self_updated.cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(cluster_id_.serialize(buffer));
            return cost_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return cluster_id_.serialized_length() + cost_.serialized_length();
        }

        static inline uint8_t serialized_length(const notification::SelfUpdated &self_updated) {
            return node::AsyncOptionalClusterIdSerializer::serialized_length(self_updated.cluster_id
                   ) +
                node::AsyncCostSerializer::serialized_length(self_updated.cost);
        }
    };

    class AsyncNeighborUpdatedEntrySerializer {
        node::AsyncNodeIdSerializer neighbor_;
        node::AsyncCostSerializer link_cost_;

      public:
        explicit AsyncNeighborUpdatedEntrySerializer(
            const notification::NeighborUpdated &neighbor_updated
        )
            : neighbor_{etl::move(neighbor_updated.neighbor_id)},
              link_cost_{etl::move(neighbor_updated.link_cost)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            SERDE_SERIALIZE_OR_RETURN(neighbor_.serialize(buffer));
            SERDE_SERIALIZE_OR_RETURN(link_cost_.serialize(buffer));
            return nb::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return neighbor_.serialized_length() + link_cost_.serialized_length();
        }

        static inline uint8_t
        serialized_length(const notification::NeighborUpdated &neighbor_updated) {
            return node::AsyncNodeIdSerializer::serialized_length(neighbor_updated.neighbor_id) +
                node::AsyncCostSerializer::serialized_length(neighbor_updated.link_cost);
        }
    };

    class AsyncNeighborRemovedEntrySerializer {
        node::AsyncNodeIdSerializer neighbor_id_;

      public:
        explicit AsyncNeighborRemovedEntrySerializer(
            const notification::NeighborRemoved &neighbor_removed
        )
            : neighbor_id_{etl::move(neighbor_removed.neighbor_id)} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return neighbor_id_.serialize(buffer);
        }

        inline uint8_t serialized_length() const {
            return neighbor_id_.serialized_length();
        }

        static inline uint8_t
        serialized_length(const notification::NeighborRemoved &neighbor_removed) {
            return node::AsyncNodeIdSerializer::serialized_length(neighbor_removed.neighbor_id);
        }
    };

    struct FrameReceivedEntry {};

    class AsyncFrameReceivedEntrySerializer {
      public:
        explicit AsyncFrameReceivedEntrySerializer(const notification::FrameReceived &) {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::SerializeResult> serialize(W &buffer) {
            return nb::SerializeResult::Ok;
        }

        inline uint8_t serialized_length() const {
            return 0;
        }

        static inline uint8_t serialized_length(const notification::FrameReceived &) {
            return 0;
        }
    };

    using AsyncNodeNotificationEntrySerializer = nb::ser::Variant<
        AsyncSelfUpdateEntrySerializer,
        AsyncNeighborUpdatedEntrySerializer,
        AsyncNeighborRemovedEntrySerializer,
        AsyncFrameReceivedEntrySerializer>;

    struct NodeSubscriptionFrame {};

    class AsyncNodeSubscriptionFrameDeserializer {

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<nb::DeserializeResult> deserialize(R &buffer) {
            return nb::DeserializeResult::Ok;
        }

        inline NodeSubscriptionFrame result() const {
            return NodeSubscriptionFrame{};
        }
    };

    struct NodoNotificationFrameMetadata {
        uint8_t entry_count;
        uint8_t serialized_length;
    };

    inline NodoNotificationFrameMetadata get_frame_metadata(notification::NotificationService &nts
    ) {
        uint8_t entry_count = nts.size();
        if (entry_count == 0) {
            return {.entry_count = 0, .serialized_length = 0};
        }

        uint8_t entry_length = etl::accumulate(
            nts.begin(), nts.end(), 0,
            [](uint8_t acc, const notification::LocalNotification &notification) {
                return acc + AsyncNodeNotificationEntrySerializer::serialized_length(notification);
            }
        );

        uint8_t length = entry_length + AsyncEntryCountSerializer::serialized_length(entry_count);
        return {.entry_count = entry_count, .serialized_length = length};
    }

    inline void serialize_frame(
        frame::FrameBufferWriter &writer,
        NodoNotificationFrameMetadata metadata,
        notification::NotificationService &nts
    ) {
        writer.serialize_all_at_once(AsyncEntryCountSerializer{metadata.entry_count});
        for (const auto &notification : nts) {
            writer.serialize_all_at_once(AsyncNodeNotificationEntrySerializer{notification});
        }
        FASSERT(writer.is_all_written());
    }
} // namespace net::observer
