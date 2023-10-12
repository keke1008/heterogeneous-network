#pragma once

#include <nb/stream.h>

namespace net::link::wifi {
    class DiscardUnknownMessage {
        nb::stream::DiscardSingleLineWritableBuffer discarder_;

      public:
        inline nb::Poll<void> execute(nb::stream::ReadableStream &stream) {
            return discarder_.write_all_from(stream);
        }
    };
} // namespace net::link::wifi
