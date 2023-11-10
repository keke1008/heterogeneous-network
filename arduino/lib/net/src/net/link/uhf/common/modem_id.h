#pragma once

#include "../../address.h"
#include <etl/array.h>
#include <logger.h>
#include <nb/buf.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>
#include <net/link/media.h>
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

        explicit ModemId(etl::span<const uint8_t, 2> value) {
            auto id = serde::hex::deserialize<uint8_t>(value);
            ASSERT(id.has_value());
            value_ = id.value();
        }

        ModemId(const etl::array<uint8_t, 2> &value) {
            auto id = serde::hex::deserialize<uint8_t>(value);
            ASSERT(id.has_value());
            value_ = id.value();
        }

        inline constexpr ModemId(const uint8_t id) : value_{id} {}

        static inline constexpr ModemId broadcast() {
            return ModemId{0x00};
        }

        inline bool is_broadcast() const {
            return value_ == 0x00;
        }

        explicit ModemId(const Address &addres) {
            ASSERT(addres.type() == AddressType::UHF);
            ASSERT(addres.address().size() == 1);
            value_ = addres.address()[0];
        }

        explicit ModemId(LinkAddress &address)
            : ModemId{etl::visit(
                  util::Visitor{
                      [](LinkUnicastAddress &address) { return ModemId(address.address); },
                      [](LinkBroadcastAddress &address) { return ModemId::broadcast(); },
                  },
                  address.variant()
              )} {}

        explicit operator LinkAddress() const {
            if (is_broadcast()) {
                return LinkAddress{AddressType::UHF};
            } else {
                return LinkAddress{Address{AddressType::UHF, {value_}}};
            }
        }

        bool operator==(const ModemId &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const ModemId &other) const {
            return value_ != other.value_;
        }

        explicit operator Address() const {
            return Address{AddressType::UHF, {value_}};
        }

        etl::array<uint8_t, 2> span() const {
            return serde::hex::serialize(value_);
        }
    };

    struct ModemIdParser {
        ModemId parse(nb::buf::BufferSplitter &splitter) {
            auto span = splitter.split_nbytes<2>();
            return ModemId{span};
        }
    };
}; // namespace net::link::uhf
