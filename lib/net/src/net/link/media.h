#pragma once

#include "./serial.h"
#include "./uhf.h"
#include "./wifi.h"
#include <etl/variant.h>
#include <util/rand.h>
#include <util/time.h>

namespace net::link {
    class MediaExecutor {
        etl::variant<uhf::UHFExecutor, wifi::WifiExecutor, serial::SerialExecutor> executor_;

      public:
        MediaExecutor() = delete;
        MediaExecutor(const MediaExecutor &) = delete;
        MediaExecutor(MediaExecutor &&) = default;
        MediaExecutor &operator=(const MediaExecutor &) = delete;
        MediaExecutor &operator=(MediaExecutor &&) = default;

        static MediaExecutor createUHF(nb::stream::ReadableWritableStream &stream);
        static MediaExecutor createWifi(nb::stream::ReadableWritableStream &stream);
        static MediaExecutor createSerial(nb::stream::ReadableWritableStream &stream);

        inline bool is_supported_address_type(AddressType type) const;

        nb::Poll<nb::Future<DataWriter>>
        send(const Address &address, const frame::BodyLength length);

        nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            return etl::visit(
                util::Visitor{
                    [&](uhf::UHFExecutor &executor) { return executor.execute(time, rand); },
                    [&](auto &executor) { return executor.execute(); },
                },
                executor_
            );
        }
    };
} // namespace net::link
