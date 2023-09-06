#pragma once

#include "../command/cs.h"
#include "../command/dt.h"
#include "./data_transmission/carrier_sense.h"
#include <etl/variant.h>
#include <nb/lock.h>

namespace net::link::uhf {
    template <typename Serial>
    class DataTransmissionTask {
        enum class State { CarrierSense, DataTransmisson } state_{State::CarrierSense};

        nb::lock::Guard<memory::Owned<Serial>> serial_;

        data_transmisson::CarrierSenseExecutor cs_executor_;
        DTExecutor<Serial> dt_executor_;

      public:
        inline DataTransmissionTask(
            nb::lock::Guard<memory::Owned<Serial>> &&serial,
            ModemId dest,
            uint8_t length,
            nb::Promise<CommandWriter<Serial>> &&body
        )
            : serial_{etl::move(serial)},
              dt_executor_{dest, length, etl::move(body)} {}

        template <typename Time, typename Rand>
        nb::Poll<bool> poll(Time &time, Rand rand) {
            if (state_ == State::CarrierSense) {
                POLL_UNWRAP_OR_RETURN(cs_executor_.poll(serial_.get().get(), time, rand));
                state_ = State::DataTransmisson;
            }

            return dt_executor_.poll(serial_.get(), time);
        }
    };
} // namespace net::link::uhf
