#pragma once

#include "./template.h"
#include <nb/poll.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class GetSerialNumberCommand : public Command<nb::stream::EmptyStreamReader> {
      public:
        GetSerialNumberCommand() : Command{CommandName::SerialNumber} {}
    };

    class GetSerialNumberResponse : public Response<nb::stream::TinyByteWriter<9>> {
      public:
        using Response::Response;

        nb::Poll<const collection::TinyBuffer<uint8_t, 9> &&> poll() const {
            return get_body().poll();
        }
    };

    using GetSerialNumberTask = Task<GetSerialNumberCommand, GetSerialNumberResponse>;
} // namespace media::uhf::modem