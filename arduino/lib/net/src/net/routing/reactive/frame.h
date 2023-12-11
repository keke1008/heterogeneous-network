#pragma once

#include "../frame_id.h"
#include <etl/type_traits.h>
#include <net/frame.h>
#include <net/node.h>
#include <stdint.h>

namespace net::routing::reactive {
    enum class RouteDiscoveryFrameType : uint8_t {
        REQUEST = 0x01,
        REPLY = 0x02,
    };

    inline bool is_valid_frame_type(uint8_t type) {
        return type <= static_cast<uint8_t>(RouteDiscoveryFrameType::REPLY);
    }

    class AsyncFrameTypeDeserializer {
        nb::de::Bin<uint8_t> type_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            return is_valid_frame_type(type_.result()) ? nb::de::DeserializeResult::Ok
                                                       : nb::de::DeserializeResult::Invalid;
        }

        inline RouteDiscoveryFrameType result() {
            ASSERT(is_valid_frame_type(type_.result()));
            return static_cast<RouteDiscoveryFrameType>(type_.result());
        }
    };

    class AsyncFrameTypeSerialzer {
        nb::ser::Bin<uint8_t> type_;

      public:
        explicit AsyncFrameTypeSerialzer(RouteDiscoveryFrameType type)
            : type_{static_cast<uint8_t>(type)} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return type_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length();
        }
    };

    enum class ExtraSpecifier : uint8_t {
        None = 0x01,
        MediaAddress = 0x02,
    };

    inline bool is_valid_extra_specifier(uint8_t type) {
        return static_cast<uint8_t>(ExtraSpecifier::None) <= type &&
            type <= static_cast<uint8_t>(ExtraSpecifier::MediaAddress);
    }

    using AsyncExtraSpecifierDeserializer = nb::de::Enum<ExtraSpecifier, is_valid_extra_specifier>;

    using AsyncExtraSpecifierSerialzer = nb::ser::Enum<ExtraSpecifier>;

    using Extra = etl::
        variant<ExtraSpecifier, nb::EmptyMarker, etl::vector<link::Address, link::MAX_MEDIA_PORT>>;
    using AsyncExtraDeserializer = nb::de::Variant<
        AsyncExtraSpecifierDeserializer,
        nb::de::Empty,
        nb::de::Vec<link::AsyncAddressDeserializer, link::MAX_MEDIA_PORT>>;
    using AsyncExtraSerializer = nb::ser::Variant<
        AsyncExtraSpecifierSerialzer,
        nb::ser::Empty,
        nb::ser::Vec<link::AsyncAddressSerializer, link::MAX_MEDIA_PORT>>;

    struct RouteDiscoveryFrame {
        // 指定忘れに気を付けて!
        RouteDiscoveryFrameType type;
        FrameId frame_id;
        node::Cost total_cost; // 探索を開始したノードから，送信元のノードまでのコスト
        node::NodeId source_id; // 探索を開始したノード
        node::NodeId target_id; // 探索の対象となるノード
        node::NodeId sender_id; // このフレームを送信したノード
        Extra extra;

        static RouteDiscoveryFrame request(
            FrameId frame_id,
            const node::NodeId &self_id,
            node::Cost self_cost,
            const node::NodeId &target_id
        ) {
            return RouteDiscoveryFrame{
                .type = RouteDiscoveryFrameType::REQUEST,
                .frame_id = frame_id,
                .total_cost = self_cost,
                .source_id = self_id,
                .target_id = target_id,
                .sender_id = self_id,
                .extra = ExtraSpecifier::None,
            };
        }

        inline RouteDiscoveryFrame repeat(node::Cost additional_cost) const {
            return RouteDiscoveryFrame{
                .type = type,
                .frame_id = frame_id,
                .total_cost = total_cost + additional_cost,
                .source_id = source_id,
                .target_id = target_id,
                .sender_id = sender_id,
                .extra = extra,
            };
        }

        template <nb::AsyncReadableWritable RW>
        inline RouteDiscoveryFrame
        reply(link::LinkService<RW> &ls, const node::NodeId &self_id, FrameId frame_id) {
            return RouteDiscoveryFrame{
                .type = RouteDiscoveryFrameType::REPLY,
                .frame_id = frame_id,
                .total_cost = node::Cost(0),
                .source_id = self_id,
                .target_id = source_id,
                .sender_id = self_id,
                .extra = etl::visit<Extra>(
                    util::Visitor{
                        [&](ExtraSpecifier specifier) -> Extra {
                            if (specifier == ExtraSpecifier::MediaAddress) {
                                etl::vector<link::Address, link::MAX_MEDIA_PORT> media_addresses;
                                ls.get_media_addresses(media_addresses);
                                return media_addresses;
                            }
                            return nb::empty_marker;
                        },
                        [](const auto &) -> Extra { return nb::empty_marker; },
                    },
                    extra
                )
            };
        }
    };

    class AsyncRouteDiscoveryFrameDeserializer {
        AsyncFrameTypeDeserializer type_;
        AsyncFrameIdDeserializer frame_id_;
        node::AsyncCostDeserializer total_cost_;
        node::AsyncNodeIdDeserializer source_id_;
        node::AsyncNodeIdDeserializer target_id_;
        node::AsyncNodeIdDeserializer sender_id_;
        AsyncExtraDeserializer extra_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(frame_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(total_cost_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(source_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(target_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(sender_id_.deserialize(r));
            return extra_.deserialize(r);
        }

        inline RouteDiscoveryFrame result() {
            return RouteDiscoveryFrame{
                .type = type_.result(),
                .frame_id = frame_id_.result(),
                .total_cost = total_cost_.result(),
                .source_id = source_id_.result(),
                .target_id = target_id_.result(),
                .sender_id = sender_id_.result(),
                .extra = extra_.result(),
            };
        }
    };

    class AsyncRouteDiscoveryFrameSerializer {
        AsyncFrameTypeSerialzer type_;
        AsyncFrameIdSerializer frame_id_;
        node::AsyncCostSerializer total_cost_;
        node::AsyncNodeIdSerializer source_id_;
        node::AsyncNodeIdSerializer target_id_;
        node::AsyncNodeIdSerializer sender_id_;
        AsyncExtraSerializer extra_;

      public:
        explicit AsyncRouteDiscoveryFrameSerializer(const RouteDiscoveryFrame &frame)
            : type_{frame.type},
              frame_id_{frame.frame_id},
              total_cost_{frame.total_cost},
              source_id_{frame.source_id},
              target_id_{frame.target_id},
              sender_id_{frame.sender_id},
              extra_{frame.extra} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(frame_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(total_cost_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(target_id_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(sender_id_.serialize(w));
            return extra_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + frame_id_.serialized_length() +
                total_cost_.serialized_length() + source_id_.serialized_length() +
                target_id_.serialized_length() + sender_id_.serialized_length() +
                extra_.serialized_length();
        }
    };
}; // namespace net::routing::reactive
