#pragma once

#include "./initialization.h"
#include "./task.h"

namespace net::link::uhf {
    class UhfInteractor {
        TaskExecutor executor_;
        etl::optional<Initializer> initializer_;
        etl::optional<ModemId> self_id_;

      public:
        // InitializerがTaskExecutorを参照するため，Copy不可
        UhfInteractor() = delete;
        UhfInteractor(const UhfInteractor &) = delete;
        UhfInteractor(UhfInteractor &&) = delete;
        UhfInteractor &operator=(const UhfInteractor &) = delete;
        UhfInteractor &operator=(UhfInteractor &&) = delete;

        inline UhfInteractor(
            nb::stream::ReadableWritableStream &stream,
            const memory::StaticRef<FrameBroker> &broker
        )
            : executor_{stream, broker},
              initializer_{Initializer{executor_}} {}

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::UHF;
        }

        inline nb::Poll<Address> get_address() const {
            return self_id_ ? nb::ready(Address{*self_id_}) : nb::pending;
        }

        void execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            if (initializer_.has_value()) {
                auto poll = initializer_->execute(frame_service, time, rand);
                if (poll.is_ready()) {
                    self_id_ = poll.unwrap();
                    initializer_.reset();
                }
            }

            executor_.execute(frame_service, time, rand);
        }
    };
} // namespace net::link::uhf
