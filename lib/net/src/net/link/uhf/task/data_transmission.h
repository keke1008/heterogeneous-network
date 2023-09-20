#pragma once

#include "../command/cs.h"
#include "../command/dt.h"
#include "./data_transmission/carrier_sense.h"
#include <etl/variant.h>
#include <nb/lock.h>
#include <nb/poll.h>

namespace net::link::uhf {
    class DataTransmissionTask {
        enum class State : uint8_t {
            CarrierSense,
            DataTransmisson,
        } state_{State::CarrierSense};

        data_transmisson::CarrierSenseExecutor cs_executor_;
        DTExecutor dt_executor_;
        nb::Promise<bool> result_;

      public:
        inline DataTransmissionTask(
            ModemId dest,
            uint8_t length,
            nb::Promise<CommandWriter> &&body,
            nb::Promise<bool> &&result
        )
            : dt_executor_{dest, length, etl::move(body)},
              result_{etl::move(result)} {}

        template <typename Rand>
        nb::Poll<void>
        poll(nb::stream::ReadableWritableStream &stream, util::Time &time, Rand rand) {
            if (state_ == State::CarrierSense) {
                POLL_UNWRAP_OR_RETURN(cs_executor_.poll(stream, time, rand));
                state_ = State::DataTransmisson;
            }

            bool result = POLL_UNWRAP_OR_RETURN(dt_executor_.poll(stream, time));
            result_.set_value(result);
            return nb::ready();
        }
    };
} // namespace net::link::uhf
