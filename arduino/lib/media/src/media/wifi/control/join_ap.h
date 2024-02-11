#pragma once

#include "./generic.h"
#include <nb/poll.h>
#include <nb/serde.h>

namespace media::wifi {
    class AsyncJoinApCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer command_{R"(AT+CWJAP=")"};
        nb::ser::AsyncSpanSerializer<32> ssid_;
        nb::ser::AsyncStaticSpanSerializer comma_{R"(",")"};
        nb::ser::AsyncSpanSerializer<64> password_;
        nb::ser::AsyncStaticSpanSerializer trailer{"\"\r\n"};

      public:
        template <typename... Args>
        explicit AsyncJoinApCommandSerializer(
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password
        )
            : ssid_{ssid},
              password_{password} {}

        template <nb::AsyncWritable W>
        nb::Poll<nb::SerializeResult> serialize(W &rw) {
            SERDE_SERIALIZE_OR_RETURN(command_.serialize(rw));
            SERDE_SERIALIZE_OR_RETURN(ssid_.serialize(rw));
            SERDE_SERIALIZE_OR_RETURN(comma_.serialize(rw));
            SERDE_SERIALIZE_OR_RETURN(password_.serialize(rw));
            return trailer.serialize(rw);
        }
    };

    template <nb::AsyncReadable R, nb::AsyncWritable W>
    struct JoinApControl : public GenericEmptyResponseControl<R, W, AsyncJoinApCommandSerializer> {
        explicit JoinApControl(
            nb::Promise<bool> &&promise,
            memory::Static<W> &writable,
            etl::span<const uint8_t> ssid,
            etl::span<const uint8_t> password
        )
            : GenericEmptyResponseControl<R, W, AsyncJoinApCommandSerializer>{
                  writable, etl::move(promise), WifiResponseMessage::Ok, ssid, password
              } {}
    };

} // namespace media::wifi
