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

        inline UhfInteractor(nb::stream::ReadableWritableStream &stream, const FrameBroker &broker)
            : executor_{stream, broker},
              initializer_{Initializer{executor_}} {}

        inline bool is_unicast_supported(AddressType type) const {
            return type == AddressType::UHF;
        }

        inline bool is_broadcast_supported(AddressType type) const {
            return type == AddressType::UHF;
        }

        inline MediaInfo get_media_info() const {
            return MediaInfo{
                .address_type = AddressType::UHF,
                .address = self_id_ ? etl::optional(Address{*self_id_}) : etl::nullopt};
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
