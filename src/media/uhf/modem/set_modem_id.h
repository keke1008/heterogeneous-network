#pragma once

#include "./command.h"
#include "./modem_id.h"
#include "./response.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class SetModemIdCommand : public Command<nb::stream::TinyByteReader<2>> {
      public:
        SetModemIdCommand(CommandName name, ModemId id)
            : Command<nb::stream::TinyByteReader<2>>{name, id.serialize()} {}
    };

    class SetModemIdResponse : public Response<nb::stream::TinyByteWriter<2>> {
      public:
        nb::Poll<ModemId> poll() const {
            const auto &&body_ = POLL_UNWRAP_OR_RETURN(get_body().poll());
            return ModemId{body_};
        }
    };
} // namespace media::uhf::modem
