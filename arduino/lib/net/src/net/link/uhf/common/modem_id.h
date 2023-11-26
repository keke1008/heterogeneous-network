#pragma once

#include "../../address.h"
#include <etl/array.h>
#include <logger.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/serde.h>
#include <net/link/media.h>
#include <serde/hex.h>
#include <util/visitor.h>

namespace net::link::uhf {
    class ModemId {
        uint8_t value_;

      public:
        ModemId() = delete;
        ModemId(const ModemId &) = default;
        ModemId(ModemId &&) = default;
        ModemId &operator=(const ModemId &) = default;
        ModemId &operator=(ModemId &&) = default;

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

        inline uint8_t get() const {
            return value_;
        }
    };

    class AsyncModemIdDeserializer {
        nb::de::Hex<uint8_t> id_;

      public:
        inline ModemId result() {
            return ModemId{id_.result()};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &reader) {
            return id_.deserialize(reader);
        }
    };

    class AsyncModemIdSerializer {
        nb::ser::Hex<uint8_t> id_;

      public:
        inline AsyncModemIdSerializer(const ModemId &id) : id_{id.get()} {}

        template <nb::ser::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writer) {
            return id_.serialize(writer);
        }

        inline constexpr uint8_t serialized_length() const {
            return id_.serialized_length();
        }
    };
} // namespace net::link::uhf
