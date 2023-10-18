#pragma once

#include <net/link.h>

namespace net::routing {
    class NodeId {
        link::Address address_;

      public:
        static constexpr uint8_t MAX_LENGTH = 1 + link::Address::MAX_LENGTH;

        explicit NodeId(const link::Address &address) : address_(address) {}

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
            builder.append(address_);
        }
    };

    struct NodeIdParser {
        NodeId parse(nb::buf::BufferSplitter &splitter) {
            return NodeId{
                splitter.parse<link::AddressDeserializer>(),
            };
        }
    };

    class Cost {
        uint8_t value_;

      public:
        static constexpr uint8_t LENGTH = 1;

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

} // namespace net::routing
