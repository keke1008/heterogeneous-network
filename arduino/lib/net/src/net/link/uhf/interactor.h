#pragma once

#include "./initialization.h"
#include "./task.h"

namespace net::link::uhf {
    template <nb::AsyncReadableWritable RW>
    class UhfInteractor {
        TaskExecutor<RW> executor_;
        etl::optional<Initializer<RW>> initializer_{Initializer<RW>{}};
        etl::optional<ModemId> self_id_;

      public:
        UhfInteractor() = delete;
        UhfInteractor(const UhfInteractor &) = delete;
        UhfInteractor(UhfInteractor &&) = delete;
        UhfInteractor &operator=(const UhfInteractor &) = delete;
        UhfInteractor &operator=(UhfInteractor &&) = delete;

        inline UhfInteractor(RW &stream, const FrameBroker &broker) : executor_{stream, broker} {}

        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            return AddressTypeSet{AddressType::UHF};
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            return AddressTypeSet{AddressType::UHF};
        }

        inline MediaInfo get_media_info() const {
            return MediaInfo{
                .address_type = AddressType::UHF,
                .address = self_id_ ? etl::optional(Address{*self_id_}) : etl::nullopt};
        }

        void execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            if (initializer_.has_value()) {
                auto poll = initializer_->execute(frame_service, executor_, time, rand);
                if (poll.is_ready()) {
                    self_id_ = poll.unwrap();
                    initializer_.reset();
                }
            }

            executor_.execute(frame_service, time, rand);
        }
    };
} // namespace net::link::uhf
