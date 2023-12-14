#pragma once

#include <etl/type_traits.h>
#include <net/frame.h>
#include <net/node.h>
#include <stdint.h>

namespace net::discovery {
    enum class DiscoveryFrameType : uint8_t {
        REQUEST = 0x01,
        REPLY = 0x02,
    };

    inline bool is_valid_frame_type(uint8_t type) {
        return type <= static_cast<uint8_t>(DiscoveryFrameType::REPLY);
    }

    using AsyncFrameTypeSerializer = nb::ser::Enum<DiscoveryFrameType>;
    using AsyncFrameTypeDeserializer = nb::de::Enum<DiscoveryFrameType, is_valid_frame_type>;

    enum class ExtraSpecifier : uint8_t {
        None = 0x01,
        MediaAddress = 0x02,
    };

    inline bool is_valid_extra_specifier(uint8_t type) {
        return static_cast<uint8_t>(ExtraSpecifier::None) <= type &&
            type <= static_cast<uint8_t>(ExtraSpecifier::MediaAddress);
    }

    using AsyncExtraSpecifierDeserializer = nb::de::Enum<ExtraSpecifier, is_valid_extra_specifier>;

    using AsyncExtraSpecifierSerializer = nb::ser::Enum<ExtraSpecifier>;

    using Extra = etl::variant<nb::EmptyMarker, etl::vector<link::Address, link::MAX_MEDIA_PORT>>;
    using AsyncExtraDeserializer = nb::de::
        Variant<nb::de::Empty, nb::de::Vec<link::AsyncAddressDeserializer, link::MAX_MEDIA_PORT>>;
    using AsyncExtraSerializer = nb::ser::
        Variant<nb::ser::Empty, nb::ser::Vec<link::AsyncAddressSerializer, link::MAX_MEDIA_PORT>>;

    struct DiscoveryFrame {
        frame::FrameId frame_id;
        node::Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        node::NodeId source_id; // 探索を開始したノード
        node::NodeId target_id; // 探索の対象となるノード
        node::NodeId sender_id; // このフレームを送信したノード
        etl::variant<ExtraSpecifier, Extra> extra;

        inline constexpr DiscoveryFrameType type() const {
            return etl::holds_alternative<ExtraSpecifier>(extra) ? DiscoveryFrameType::REQUEST
                                                                 : DiscoveryFrameType::REPLY;
        }

        static DiscoveryFrame request(
            frame::FrameId frame_id,
            const node::NodeId &local_id,
            node::Cost self_cost,
            const node::NodeId &target_id
        ) {
            return DiscoveryFrame{
                .frame_id = frame_id,
                .total_cost = self_cost,
                .source_id = local_id,
                .target_id = target_id,
                .sender_id = local_id,
                .extra = ExtraSpecifier::None,
            };
        }

        inline DiscoveryFrame repeat(node::Cost additional_cost) const {
            return DiscoveryFrame{
                .frame_id = frame_id,
                .total_cost = total_cost + additional_cost,
                .source_id = source_id,
                .target_id = target_id,
                .sender_id = sender_id,
                .extra = extra,
            };
        }

        template <nb::AsyncReadableWritable RW>
        inline DiscoveryFrame
        reply(link::LinkService<RW> &ls, const node::NodeId &self_id, frame::FrameId frame_id) {
            ASSERT(type() == DiscoveryFrameType::REQUEST);
            Extra extra;
            if (etl::get<ExtraSpecifier>(this->extra) == ExtraSpecifier::MediaAddress) {
                etl::vector<link::Address, link::MAX_MEDIA_PORT> media_addresses;
                ls.get_media_addresses(media_addresses);
                extra = media_addresses;
            } else {
                extra = nb::empty_marker;
            }

            return DiscoveryFrame{
                .frame_id = frame_id,
                .total_cost = node::Cost(0),
                .source_id = self_id,
                .target_id = source_id,
                .sender_id = self_id,
                .extra = extra,
            };
        }
    };

    class AsyncDiscoveryFrameDeserializer {
        AsyncFrameTypeDeserializer type_;
        frame::AsyncFrameIdDeserializer frame_id_;
        node::AsyncCostDeserializer total_cost_;
        node::AsyncNodeIdDeserializer source_id_;
        node::AsyncNodeIdDeserializer destination_id_;
        node::AsyncNodeIdDeserializer sender_id_;
        etl::optional<nb::de::Union<AsyncExtraSpecifierDeserializer, AsyncExtraDeserializer>>
            extra_;

      public:
        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(frame_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(total_cost_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(destination_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(sender_id_.deserialize(r));
            if (!extra_.has_value()) {
                auto type = type_.result();
                if (type == DiscoveryFrameType::REQUEST) {
                    extra_.emplace(AsyncExtraSpecifierDeserializer{});
                } else {
                    extra_.emplace(AsyncExtraDeserializer{});
                }
            }
            return extra_->deserialize(r);
        }

        inline DiscoveryFrame result() const {
            return DiscoveryFrame{
                .frame_id = frame_id_.result(),
                .total_cost = total_cost_.result(),
                .source_id = source_id_.result(),
                .target_id = destination_id_.result(),
                .sender_id = sender_id_.result(),
                .extra = extra_->result(),
            };
        }
    };

    class AsyncDiscoveryFrameSerializer {
        AsyncFrameTypeSerializer type_;
        frame::AsyncFrameIdSerializer frame_id_;
        node::AsyncCostSerializer total_cost_;
        node::AsyncNodeIdSerializer source_id_;
        node::AsyncNodeIdSerializer destination_id_;
        node::AsyncNodeIdSerializer sender_id_;
        nb::ser::Union<AsyncExtraSpecifierSerializer, AsyncExtraSerializer> extra_;

      public:
        explicit AsyncDiscoveryFrameSerializer(const DiscoveryFrame &frame)
            : type_{DiscoveryFrameType::REQUEST},
              frame_id_{frame.frame_id},
              total_cost_{frame.total_cost},
              source_id_{frame.source_id},
              destination_id_{frame.target_id},
              sender_id_{frame.sender_id},
              extra_{frame.extra} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(frame_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(total_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(destination_id_.serialize(w));
            return sender_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return frame_id_.serialized_length() + total_cost_.serialized_length() +
                source_id_.serialized_length() + destination_id_.serialized_length() +
                sender_id_.serialized_length();
        }
    };
}; // namespace net::discovery
