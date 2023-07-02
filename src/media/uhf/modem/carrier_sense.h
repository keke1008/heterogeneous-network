#pragma once

#include "./error.h"
#include "./generic.h"
#include <nb/poll.h>
#include <nb/result.h>
#include <nb/stream.h>

namespace media::uhf::modem {
    class CarrierSenseCommand : public Command<nb::stream::EmptyStreamReader> {
      public:
        CarrierSenseCommand() : Command{CommandName::CarrierSense} {}
    };

    class CarrierSenseResponse : public Response<nb::stream::TinyByteWriter<2>> {
      public:
        using Response::Response;

        nb::Poll<ModemResult<nb::Empty>> poll() const {
            const auto &response = POLL_UNWRAP_OR_RETURN(get_body().poll());
            if (response.get<0>() == 'E' && response.get<1>() == 'N') {
                return nb::Ok{nb::Empty{}};
            } else {
                return nb::Err{ModemError::carrier_sense()};
            }
        }
    };
} // namespace media::uhf::modem
