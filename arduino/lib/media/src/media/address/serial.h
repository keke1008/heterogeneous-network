#pragma once

#include "./single_byte.h"

namespace media {
    class SerialAddress : public SingleByteAddress<net::link::AddressType::Serial> {
        uint8_t value_;

      public:
        using SingleByteAddress::SingleByteAddress;
        using SingleByteAddress::operator net::link::Address;

        inline static constexpr uint8_t SIZE = 1;
    };
} // namespace media
