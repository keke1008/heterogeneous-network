#pragma once

#include "./serial.h"
#include "./uhf.h"
#include "./wifi.h"
#include <etl/variant.h>
#include <util/rand.h>
#include <util/time.h>

namespace net::link {
    class MediaExecutor {
        etl::variant<uhf::UHFFacade, wifi::WifiFacade, serial::SerialExecutor> executor_;

      public:
        MediaExecutor() = delete;
        MediaExecutor(const MediaExecutor &) = delete;
        MediaExecutor(MediaExecutor &&) = default;
        MediaExecutor &operator=(const MediaExecutor &) = delete;
        MediaExecutor &operator=(MediaExecutor &&) = default;

        static MediaExecutor createUHF(nb::stream::ReadableWritableStream &stream);
        static MediaExecutor createWifi(nb::stream::ReadableWritableStream &stream);
        static MediaExecutor createSerial(nb::stream::ReadableWritableStream &stream);

        inline bool is_supported_address_type(AddressType type) const {
            return etl::visit(
                [type](auto &executor) { return executor.is_supported_address_type(type); },
                executor_
            );
        }

        nb::Poll<FrameTransmission>
        send_frame(const Address &address, const frame::BodyLength length) {
            return etl::visit(
                [&](auto &executor) { return executor.send_data(address, length); }, executor_
            );
        }

        nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            return etl::visit(
                util::Visitor{
                    [&](uhf::UHFFacade &executor) { return executor.execute(time, rand); },
                    [&](auto &executor) { return executor.execute(); },
                },
                executor_
            );
        }
    };
} // namespace net::link
