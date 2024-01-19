#pragma once

#include "./single_byte.h"

namespace media {
    class ModemId : public SingleByteAddress<net::link::AddressType::UHF> {
        uint8_t value_;

      public:
        using SingleByteAddress::SingleByteAddress;
        using SingleByteAddress::operator net::link::Address;

        static inline ModemId broadcast() {
            return ModemId{0x00};
        }

        inline bool is_broadcast() const {
            return value_ == 0x00;
        }
    };
} // namespace media
