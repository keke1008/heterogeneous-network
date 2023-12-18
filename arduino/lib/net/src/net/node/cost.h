#pragma once

#include <nb/serde.h>
#include <util/time.h>

namespace net::node {
    class Cost {
        uint16_t value_;

      public:
        static constexpr uint8_t LENGTH = 2;

        inline explicit constexpr Cost(uint16_t value) : value_{value} {}

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

        inline constexpr Cost operator+(const Cost &other) const {
            return Cost{static_cast<uint16_t>(value_ + other.value_)};
        }

        inline constexpr uint16_t value() const {
            return value_;
        }

        explicit inline operator util::Duration() const {
            return util::Duration::from_millis(value_);
        }

        inline friend logger::log::Printer &
        operator<<(logger::log::Printer &printer, const Cost &cost) {
            return printer << '(' << cost.value_ << ')';
        }
    };

    class AsyncCostDeserializer {
        nb::de::Bin<uint16_t> value_;

      public:
        inline Cost result() const {
            return Cost{value_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return value_.deserialize(r);
        }
    };

    class AsyncCostSerializer {
        nb::ser::Bin<uint16_t> value_;

      public:
        explicit AsyncCostSerializer(const Cost &cost) : value_{cost.value()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &w) {
            return value_.serialize(w);
        }

        constexpr inline uint8_t serialized_length() const {
            return value_.serialized_length();
        }
    };
} // namespace net::node
