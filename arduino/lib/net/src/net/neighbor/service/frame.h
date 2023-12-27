#pragma once

#include <net/frame.h>
#include <net/node.h>

namespace net::neighbor::service {
    enum class FrameType : uint8_t {
        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者の情報
        // 3. 送信者のノードコスト
        // 4. リンクコスト
        HELLO = 0x01,

        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者の情報
        // 3. 送信者のノードコスト
        // 4. リンクコスト
        HELLO_ACK = 0x02,

        // # フレームフォーマット
        //
        // 1. フレームタイプ (1 byte)
        // 2. 送信者のノードID
        GOODBYE = 0x03,
    };

    inline bool is_valid_frame_type(uint8_t type) {
        return type <= static_cast<uint8_t>(FrameType::GOODBYE);
    }

    using AsyncFrameTypeDeserializer = nb::de::Enum<FrameType, is_valid_frame_type>;
    using AsyncFrameTypeSerializer = nb::ser::Enum<FrameType>;

    struct HelloFrame {
        bool is_ack;
        node::Source source;
        node::Cost node_cost;
        node::Cost link_cost;

        inline FrameType frame_type() const {
            return is_ack ? FrameType::HELLO_ACK : FrameType::HELLO;
        }
    };

    class AsyncHelloFrameSerializer {
        AsyncFrameTypeSerializer type_;
        node::AsyncSourceSerializer source_;
        node::AsyncCostSerializer node_cost_;
        node::AsyncCostSerializer link_cost_;

      public:
        AsyncHelloFrameSerializer(const HelloFrame &frame)
            : type_{frame.frame_type()},
              source_{frame.source},
              node_cost_{frame.node_cost},
              link_cost_{frame.link_cost} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(source_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(node_cost_.serialize(w));
            return link_cost_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + source_.serialized_length() +
                node_cost_.serialized_length() + link_cost_.serialized_length();
        }
    };

    class AsyncHelloFrameDeserializer {
        bool is_ack_;
        node::AsyncSourceDeserializer source_;
        node::AsyncCostDeserializer node_cost_;
        node::AsyncCostDeserializer link_cost_;

      public:
        explicit AsyncHelloFrameDeserializer(bool is_ack) : is_ack_{is_ack} {}

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(source_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(node_cost_.deserialize(r));
            return link_cost_.deserialize(r);
        }

        inline HelloFrame result() {
            return HelloFrame{
                .is_ack = is_ack_,
                .source = source_.result(),
                .node_cost = node_cost_.result(),
                .link_cost = link_cost_.result(),
            };
        }
    };

    struct GoodbyeFrame {
        node::NodeId sender_id;

        inline constexpr FrameType frame_type() const {
            return FrameType::GOODBYE;
        }
    };

    class AsyncGoodbyeFrameSerializer {
        AsyncFrameTypeSerializer type_;
        node::AsyncNodeIdSerializer sender_id_;

      public:
        AsyncGoodbyeFrameSerializer(FrameType type, const node::NodeId &sender_id)
            : type_{type},
              sender_id_{sender_id} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            SERDE_SERIALIZE_OR_RETURN(type_.serialize(w));
            SERDE_SERIALIZE_OR_RETURN(sender_id_.serialize(w));
            return sender_id_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return type_.serialized_length() + sender_id_.serialized_length();
        }
    };

    class AsyncGoodbyeFrameDeserilizer {
        node::AsyncNodeIdDeserializer sender_id_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return sender_id_.deserialize(r);
        }

        inline GoodbyeFrame result() {
            return GoodbyeFrame{
                .sender_id = sender_id_.result(),
            };
        }
    };

    using NeighborFrame = etl::variant<HelloFrame, GoodbyeFrame>;

    class AsyncNeighborFrameDeserializer {
        AsyncFrameTypeDeserializer type_;
        etl::variant<etl::monostate, AsyncHelloFrameDeserializer, AsyncGoodbyeFrameDeserilizer>
            frame_;

      public:
        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            if (etl::holds_alternative<etl::monostate>(frame_)) {
                SERDE_DESERIALIZE_OR_RETURN(type_.deserialize(r));
                switch (type_.result()) {
                case FrameType::HELLO:
                    frame_.emplace<AsyncHelloFrameDeserializer>(false);
                    break;
                case FrameType::HELLO_ACK:
                    frame_.emplace<AsyncHelloFrameDeserializer>(true);
                    break;
                case FrameType::GOODBYE:
                    frame_.emplace<AsyncGoodbyeFrameDeserilizer>();
                    break;
                default:
                    return nb::de::DeserializeResult::Invalid;
                }
            }

            return etl::visit(
                util::Visitor{
                    [&](etl::monostate) { return nb::de::DeserializeResult::Invalid; },
                    [&](auto &frame) { return frame.deserialize(r); },
                },
                frame_
            );
        }

        inline NeighborFrame result() {
            return etl::visit(
                util::Visitor{
                    [&](etl::monostate) -> NeighborFrame { FPANIC("No Frame"); },
                    [&](auto &frame) -> NeighborFrame { return frame.result(); },
                },
                frame_
            );
        }
    };

    class AsyncNeighborFrameSerializer {
        etl::variant<AsyncHelloFrameSerializer, AsyncGoodbyeFrameSerializer> frame_;

        template <typename T>
        static etl::variant<AsyncHelloFrameSerializer, AsyncGoodbyeFrameSerializer>
        into_variant(T &&frame) {
            return etl::visit(
                util::Visitor{
                    [&](const HelloFrame &frame) {
                        return etl::variant<AsyncHelloFrameSerializer, AsyncGoodbyeFrameSerializer>{
                            AsyncHelloFrameSerializer{frame}
                        };
                    },
                    [&](const GoodbyeFrame &frame) {
                        return etl::variant<AsyncHelloFrameSerializer, AsyncGoodbyeFrameSerializer>{
                            AsyncGoodbyeFrameSerializer{frame.frame_type(), frame.sender_id}
                        };
                    },
                },
                NeighborFrame{etl::forward<T>(frame)}
            );
        }

      public:
        template <typename T>
        explicit AsyncNeighborFrameSerializer(T &&frame)
            : frame_{into_variant(etl::forward<T>(frame))} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return etl::visit([&](auto &frame) { return frame.serialize(w); }, frame_);
        }

        constexpr inline uint8_t serialized_length() const {
            return etl::visit([&](const auto &frame) { return frame.serialized_length(); }, frame_);
        }
    };
} // namespace net::neighbor::service