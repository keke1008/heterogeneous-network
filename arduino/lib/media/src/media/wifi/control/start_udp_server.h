#pragma once

#include "../frame.h"
#include "./generic.h"

namespace media::wifi {
    class AsyncStartUdpServerCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer command_{R"(AT+CIPSTART="UDP","0.0.0.0",12345,)"};
        AsyncUdpPortSerializer local_port_;
        nb::ser::AsyncStaticSpanSerializer trailer{",2\r\n"};

      public:
        inline explicit AsyncStartUdpServerCommandSerializer(UdpPort local_port)
            : local_port_{local_port} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &rw) {
            SERDE_SERIALIZE_OR_RETURN(command_.serialize(rw));
            SERDE_SERIALIZE_OR_RETURN(local_port_.serialize(rw));
            return trailer.serialize(rw);
        }
    };

    using StartUdpServer = AsyncControl<AsyncStartUdpServerCommandSerializer>;
} // namespace media::wifi
