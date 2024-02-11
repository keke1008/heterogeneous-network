#pragma once

#include "../frame.h"
#include "./generic.h"

namespace media::wifi {
    class AsyncStartUdpServerCommandSerializer {
        nb::ser::AsyncFlashStringSerializer command_;
        AsyncUdpPortSerializer local_port_;
        nb::ser::AsyncStaticSpanSerializer trailer{",2\r\n"};

      public:
        inline explicit AsyncStartUdpServerCommandSerializer(UdpPort local_port)
            : command_{FLASH_STRING(R"(AT+CIPSTART="UDP","0.0.0.0",12345,)")},
              local_port_{local_port} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &rw) {
            SERDE_SERIALIZE_OR_RETURN(command_.serialize(rw));
            SERDE_SERIALIZE_OR_RETURN(local_port_.serialize(rw));
            return trailer.serialize(rw);
        }
    };

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct StartUdpServerControl
        : public GenericEmptyResponseControl<R, W, AsyncStartUdpServerCommandSerializer> {
        explicit StartUdpServerControl(
            nb::Promise<bool> &&promise,
            memory::Static<W> &writable,
            UdpPort local_port
        )
            : GenericEmptyResponseControl<R, W, AsyncStartUdpServerCommandSerializer>{
                  writable, etl::move(promise), WifiResponseMessage::Ok, local_port
              } {}
    };
} // namespace media::wifi
