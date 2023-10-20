#pragma once

#include "./media.h"
#include "./serial.h"
#include "./uhf.h"
#include "./wifi.h"
#include <etl/expected.h>
#include <etl/variant.h>
#include <net/frame/service.h>
#include <util/rand.h>
#include <util/time.h>

namespace net::link {
    struct UnSupportedOperation {};

    class MediaExecutor {
        using Executor = etl::variant<uhf::UHFFacade, wifi::WifiFacade, serial::SerialExecutor>;

        Executor executor_;

        static Executor
        create_executor(nb::stream::ReadableWritableStream &stream, MediaType type) {
            switch (type) {
            case MediaType::UHF:
                return Executor{uhf::UHFFacade{stream}};
            case MediaType::Wifi:
                return Executor{wifi::WifiFacade{stream, 19073}};
            case MediaType::Serial:
                return Executor{serial::SerialExecutor{stream, SerialAddress{02}}};
            default:
                DEBUG_ASSERT(false, "Unreachable");
            }
        }

      public:
        MediaExecutor() = delete;
        MediaExecutor(const MediaExecutor &) = delete;
        MediaExecutor(MediaExecutor &&) = default;
        MediaExecutor &operator=(const MediaExecutor &) = delete;
        MediaExecutor &operator=(MediaExecutor &&) = default;

        inline explicit MediaExecutor(nb::stream::ReadableWritableStream &stream, MediaType type)
            : executor_{create_executor(stream, type)} {}

        inline bool is_supported_address_type(AddressType type) const {
            return etl::visit(
                [type](auto &executor) { return executor.is_supported_address_type(type); },
                executor_
            );
        }

        inline etl::optional<Address> get_address() const {
            return etl::visit([](auto &executor) { return executor.get_address(); }, executor_);
        }

        inline nb::Poll<void> send_frame(SendingFrame &frame) {
            return etl::visit(
                [&](auto &executor) mutable { return executor.send_frame(frame); }, executor_
            );
        }

        inline nb::Poll<Frame> receive_frame(frame::ProtocolNumber protocol_number) {
            return etl::visit(
                [=](auto &executor) { return executor.receive_frame(protocol_number); }, executor_
            );
        }

        template <net::frame::IFrameService FrameService>
        void execute(FrameService &service, util::Time &time, util::Rand &rand) {
            return etl::visit(
                util::Visitor{
                    [&](uhf::UHFFacade &executor) { return executor.execute(service, time, rand); },
                    [&](auto &executor) { return executor.execute(service); },
                },
                executor_
            );
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, UnSupportedOperation>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            if (etl::holds_alternative<wifi::WifiFacade>(executor_)) {
                auto &executor = etl::get<wifi::WifiFacade>(executor_);
                return executor.join_ap(ssid, password);
            } else {
                return etl::expected<nb::Poll<nb::Future<bool>>, UnSupportedOperation>{
                    etl::unexpect,
                };
            }
        }
    };
} // namespace net::link
