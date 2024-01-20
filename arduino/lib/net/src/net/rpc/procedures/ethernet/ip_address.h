#pragma once

#include <etl/array.h>
#include <nb/serde.h>

namespace net::rpc::ethernet {
    struct IpAddress {
        etl::array<uint8_t, 4> ip;
    };

    class AsyncIpAddressDeserializer {
        nb::de::Array<nb::de::Bin<uint8_t>, 4> ip_{4};

      public:
        inline IpAddress result() const {
            const auto &ip = ip_.result();
            return IpAddress{.ip{ip[0], ip[1], ip[2], ip[3]}};
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            return ip_.deserialize(readable);
        }
    };
} // namespace net::rpc::ethernet
