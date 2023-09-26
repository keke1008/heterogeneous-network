#pragma once

#include "../../frame.h"
#include <debug_assert.h>
#include <etl/array.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace net::link::uhf {
    class ModemIdSerializer;

    class ModemId {
        friend class ModemIdSerializer;

        uint8_t value_;

      public:
        ModemId() = delete;
        ModemId(const ModemId &) = default;
        ModemId(ModemId &&) = default;
        ModemId &operator=(const ModemId &) = default;
        ModemId &operator=(ModemId &&) = default;

        ModemId(const etl::array<uint8_t, 2> &value) {
            auto id = serde::hex::deserialize<uint8_t>(value);
            DEBUG_ASSERT(id.has_value());
            value_ = id.value();
        }

        ModemId(const uint8_t id) : value_{id} {}

        bool operator==(const ModemId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const ModemId &other) const {
            return value_ != other.value_;
        }

        explicit operator Address() const {
            return Address{AddressType::UHF, {value_}};
        }

        etl::array<uint8_t, 2> span() {
            return serde::hex::serialize(value_);
        }
    };

    class ModemIdSerializer final : public nb::stream::ReadableBuffer {
        nb::stream::FixedReadableBuffer<2> buffer_;

      public:
        ModemIdSerializer(ModemId &modem_id) : buffer_{modem_id.value_} {};

        inline nb::Poll<void> read_all_into(nb::stream::WritableStream &destination) override {
            return buffer_.read_all_into(destination);
        }
    };
}; // namespace net::link::uhf
