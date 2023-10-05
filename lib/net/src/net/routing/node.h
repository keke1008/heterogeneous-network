#pragma once

#include <net/link.h>

namespace net::routing {
    class NodeAddress final : public nb::buf::BufferWriter {
        link::Address address_;

      public:
        NodeAddress(const link::Address &address) : address_(address) {}

        inline bool operator==(const NodeAddress &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const NodeAddress &other) const {
            return address_ != other.address_;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) override {
            return address_.write_to_builder(builder);
        }
    };

    struct NodeAddressParser final : nb::buf::BufferParser<NodeAddress> {
        NodeAddress parse(nb::buf::BufferSplitter &splitter) {
            return NodeAddress{splitter.parse<link::AddressDeserializer>()};
        }
    };

    class Cost {
        uint8_t value_;

        static constexpr uint8_t INFINITE = etl::numeric_limits<uint8_t>::max();

      public:
        inline explicit constexpr Cost() : value_{INFINITE} {}

        inline explicit constexpr Cost(uint8_t value) : value_{value} {}

        static inline constexpr Cost infinite() {
            return Cost{INFINITE};
        }

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

        inline constexpr bool is_infinite() const {
            return value_ == INFINITE;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) {
            builder.append(value_);
        }
    };

    struct CostParser final : public nb::buf::BufferParser<Cost> {
        inline Cost parse(nb::buf::BufferSplitter &splitter) override {
            return Cost{splitter.split_1byte()};
        }
    };

} // namespace net::routing
