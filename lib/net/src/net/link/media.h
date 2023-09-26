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

        template <typename T>
        inline explicit MediaExecutor(T &&executor) : executor_{etl::move(executor)} {}

      public:
        MediaExecutor() = delete;
        MediaExecutor(const MediaExecutor &) = delete;
        MediaExecutor(MediaExecutor &&) = default;
        MediaExecutor &operator=(const MediaExecutor &) = delete;
        MediaExecutor &operator=(MediaExecutor &&) = default;

        static MediaExecutor createUHF(nb::stream::ReadableWritableStream &stream) {
            return MediaExecutor{uhf::UHFFacade{stream}};
        }

        static MediaExecutor createWifi(nb::stream::ReadableWritableStream &stream) {
            return MediaExecutor{wifi::WifiFacade{stream, 19073}};
        }

        static MediaExecutor createSerial(nb::stream::ReadableWritableStream &stream) {
            return MediaExecutor{serial::SerialExecutor{stream, SerialAddress{02}}};
        }

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
