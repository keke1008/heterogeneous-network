#pragma once

#include "../command/cs.h"
#include "../command/dt.h"
#include "./data_transmission/carrier_sense.h"
#include <etl/variant.h>
#include <nb/lock.h>
#include <nb/poll.h>
#include <util/rand.h>

namespace net::link::uhf {
    class DataTransmissionTask {
        enum class State : uint8_t {
            CarrierSense,
            DataTransmisson,
        } state_{State::CarrierSense};

        data_transmisson::CarrierSenseExecutor cs_executor_;
        DTExecutor dt_executor_;

      public:
        inline DataTransmissionTask(UhfFrame &&frame) : dt_executor_{etl::move(frame)} {}

        nb::Poll<void>
        poll(nb::stream::ReadableWritableStream &stream, util::Time &time, util::Rand &rand) {
            if (state_ == State::CarrierSense) {
                POLL_UNWRAP_OR_RETURN(cs_executor_.poll(stream, time, rand));
                state_ = State::DataTransmisson;
            }

            return dt_executor_.poll(stream, time);
        }
    };
} // namespace net::link::uhf
