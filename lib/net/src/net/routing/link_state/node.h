#pragma once

#include <net/link.h>

namespace net::routing::link_state {
    enum class NodeType : uint8_t {
        Node,
        Media,
    };

    struct NodeTypeWriter {
        NodeType type;

        inline void write_to_builder(nb::buf::BufferBuilder &builder) {
            builder.append(nb::buf::FormatBinary<uint8_t>{static_cast<uint8_t>(type)});
        }
    };

    struct NodeTypeParser {
        inline NodeType parse(nb::buf::BufferSplitter &splitter) {
            return static_cast<NodeType>(splitter.split_1byte());
        }
    };

    class NodeId {
        NodeType type_;
        link::Address address_;

      public:
        explicit NodeId(NodeType type, const link::Address &address)
            : type_{type},
              address_(address) {}

        inline bool operator==(const NodeId &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const NodeId &other) const {
            return address_ != other.address_;
        }

        inline uint8_t serialized_length() const {
            return 1 + address_.total_length();
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(NodeTypeWriter{type_});
            builder.append(address_);
        }
    };

    struct NodeIdParser {
        NodeId parse(nb::buf::BufferSplitter &splitter) {
            return NodeId{
                splitter.parse<NodeTypeParser>(),
                splitter.parse<link::AddressDeserializer>(),
            };
        }
    };

    class Cost {
        uint8_t value_;

      public:
        inline constexpr Cost() : value_{etl::numeric_limits<uint8_t>::max()} {}

        inline explicit constexpr Cost(uint8_t value) : value_{value} {}

        inline constexpr bool operator<(const Cost &other) const {
            return value_ < other.value_;
        }

        inline constexpr bool operator>(const Cost &other) const {
            return value_ > other.value_;
        }

        inline constexpr bool operator<=(const Cost &other) const {
            return value_ <= other.value_;
        }

        inline constexpr bool operator>=(const Cost &other) const {
            return value_ >= other.value_;
        }

        inline constexpr bool operator==(const Cost &other) const {
            return value_ == other.value_;
        }

        inline constexpr bool operator!=(const Cost &other) const {
            return value_ != other.value_;
        }

        inline constexpr uint8_t value() const {
            return value_;
        }

        inline constexpr uint8_t serialized_length() const {
            return 1;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) const {
            builder.append(value_);
        }
    };

    struct CostParser {
        inline Cost parse(nb::buf::BufferSplitter &splitter) {
            return Cost{splitter.split_1byte()};
        }
    };

} // namespace net::routing::link_state
