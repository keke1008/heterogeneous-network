#pragma once

#include "../constants.h"
#include "./frame_type.h"
#include <net/frame.h>
#include <net/routing.h>

namespace net::observer {
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

    class NodeNotificationFrameWriter {
        using NotificationCountSerializer = nb::ser::Bin<uint8_t>;

        uint8_t calculate_length(const notification::NotificationService &nts) {
            uint8_t length = 0;

            length += AsyncFrameTypeSerializer::serialized_length(FrameType::NodeNotification);

            length += NotificationCountSerializer::serialized_length(nts.size());
            for (const auto &notification : nts) {
                length += AsyncNodeNotificationEntrySerializer::serialized_length(notification);
            }

            return length;
        }

        frame::FrameBufferReader write_frame(
            const notification::NotificationService &nts,
            frame::FrameBufferWriter &&writer
        ) {
            writer.serialize_all_at_once(AsyncFrameTypeSerializer(FrameType::NodeNotification));

            writer.serialize_all_at_once(NotificationCountSerializer(nts.size()));
            for (const auto &notification : nts) {
                writer.serialize_all_at_once(AsyncNodeNotificationEntrySerializer(notification));
            }

            FASSERT(writer.is_all_written());
            return writer.create_reader();
        }

      public:
        nb::Poll<frame::FrameBufferReader> execute(
            frame::FrameService &fs,
            notification::NotificationService &nts,
            const local::LocalNodeService &lns,
            routing::RoutingSocket<FRAME_DELAY_POOL_SIZE> &socket,
            const node::Destination &destination,
            util::Rand &rand

        ) {
            uint8_t length = calculate_length(nts);

            auto &&writer = POLL_MOVE_UNWRAP_OR_RETURN(
                socket.poll_frame_writer(fs, lns, rand, destination, length)
            );
            return write_frame(nts, etl::move(writer));
        }
    };
} // namespace net::observer
