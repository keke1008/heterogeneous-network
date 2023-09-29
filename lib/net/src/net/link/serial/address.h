#pragma once

#include "../frame.h"
#include <nb/buf.h>
#include <stdint.h>

namespace net::link {
    class SerialAddress final : public nb::buf::BufferWriter {
        uint8_t address_;

      public:
        static constexpr uint8_t SIZE = 1;

        explicit SerialAddress(uint8_t address) : address_{address} {}

        explicit SerialAddress(const Address &address) {
            DEBUG_ASSERT(address.type() == AddressType::Serial);
            DEBUG_ASSERT(address.address().size() == 1);
            address_ = address.address()[0];
        }

        explicit operator Address() const {
            return Address{AddressType::Serial, {address_}};
        }

        inline bool operator==(const SerialAddress &other) const {
            return address_ == other.address_;
        }

        inline bool operator!=(const SerialAddress &other) const {
            return !(*this == other);
        }

        uint8_t get() const {
            return address_;
        }

        inline void write_to_builder(nb::buf::BufferBuilder &builder) override {
            builder.append(address_);
        }
    };

    struct SerialAddressParser final : public nb::buf::BufferParser<SerialAddress> {
        inline SerialAddress parse(nb::buf::BufferSplitter &splitter) override {
            return SerialAddress{splitter.split_1byte()};
        }
    };
} // namespace net::link
