#pragma once

#include <net/link.h>

namespace media {
    template <net::link::AddressType Type>
    class SingleByteAddress {
        uint8_t value_;

      public:
        inline constexpr SingleByteAddress(const uint8_t id) : value_{id} {}

        inline bool is_broadcast() const {
            return value_ == 0x00;
        }

        static inline bool is_convertible_address(const net::link::Address &address) {
            return address.type() == Type && address.body().size() == 1;
        }

        explicit SingleByteAddress(const net::link::Address &addres) {
            FASSERT(is_convertible_address(addres));
            value_ = addres.body().front();
        }

        explicit operator net::link::Address() const {
            return net::link::Address{Type, etl::array{value_}};
        }

        bool operator==(const SingleByteAddress &other) const {
            return value_ == other.value_;
        }

        bool operator!=(const SingleByteAddress &other) const {
            return value_ != other.value_;
        }

        inline uint8_t get() const {
            return value_;
        }
    };
} // namespace media
