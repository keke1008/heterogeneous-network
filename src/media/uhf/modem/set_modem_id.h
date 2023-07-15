#pragma once

#include "./modem_id.h"
#include "./template.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class SetEquipmentIdCommand {
        Command<nb::stream::TinyByteReader<2>> command_;

      public:
        SetEquipmentIdCommand(ModemId id) : command_{CommandName::SetEquipmentId, id.serialize()} {}

        inline constexpr decltype(auto) delegate_reader() {
            return command_.delegate_reader();
        }

        inline constexpr decltype(auto) delegate_reader() const {
            return command_.delegate_reader();
        }
    };

    class SeDestinationIdCommand {
        Command<nb::stream::TinyByteReader<2>> command_;

      public:
        SeDestinationIdCommand(ModemId id)
            : command_{CommandName::SetDestinationId, id.serialize()} {}

        inline constexpr decltype(auto) delegate_reader() {
            return command_.delegate_reader();
        }

        inline constexpr decltype(auto) delegate_reader() const {
            return command_.delegate_reader();
        }
    };

    class SetModemIdResponse : public Response<nb::stream::TinyByteWriter<2>> {
      public:
        nb::Poll<ModemId> poll() const {
            const auto &&body_ = POLL_UNWRAP_OR_RETURN(get_body().poll());
            return ModemId{body_};
        }
    };

    using SetEquipmentIdTask = Task<SetEquipmentIdCommand, SetModemIdResponse>;
    using SetDestinationIdTask = Task<SeDestinationIdCommand, SetModemIdResponse>;
} // namespace media::uhf::modem
