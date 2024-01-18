#pragma once

#include <nb/serde.h>

namespace net::link::wifi {
    class DiscardUnknownMessage {
        nb::de::AsyncDiscardingSingleLineDeserializer discarder_;

      public:
        template <nb::AsyncReadable R>
        inline nb::Poll<void> execute(R &r) {
            POLL_UNWRAP_OR_RETURN(discarder_.deserialize(r));
            return nb::ready();
        }
    };
} // namespace net::link::wifi
