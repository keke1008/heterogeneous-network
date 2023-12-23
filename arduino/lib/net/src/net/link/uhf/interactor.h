#pragma once

#include "./initialization.h"
#include "./task.h"
#include <nb/lock.h>

namespace net::link::uhf {
    template <nb::AsyncReadableWritable RW>
    class UhfInteractor {
        nb::Lock<etl::reference_wrapper<RW>> rw_;
        UhfResponseHeaderReceiver<RW> header_receiver_;
        TaskExecutor<RW> executor_;
        etl::optional<Initializer<RW>> initializer_{Initializer<RW>{}};
        etl::optional<ModemId> self_id_;

      public:
        UhfInteractor() = delete;
        UhfInteractor(const UhfInteractor &) = delete;
        UhfInteractor(UhfInteractor &&) = delete;
        UhfInteractor &operator=(const UhfInteractor &) = delete;
        UhfInteractor &operator=(UhfInteractor &&) = delete;

        inline UhfInteractor(RW &rw, const FrameBroker &broker)
            : rw_{etl::ref(rw)},
              executor_{broker} {}

        inline constexpr AddressTypeSet unicast_supported_address_types() const {
            return AddressTypeSet{AddressType::UHF};
        }

        inline constexpr AddressTypeSet broadcast_supported_address_types() const {
            return AddressTypeSet{AddressType::UHF};
        }

        inline MediaInfo get_media_info() const {
            return MediaInfo{
                .address_type = AddressType::UHF,
                .address = self_id_ ? etl::optional(Address{*self_id_}) : etl::nullopt
            };
        }

        void execute(frame::FrameService &frame_service, util::Time &time, util::Rand &rand) {
            if (initializer_.has_value()) {
                auto poll_self_id = initializer_->execute(frame_service, executor_, time, rand);
                if (poll_self_id.is_ready()) {
                    if (poll_self_id.unwrap().has_value()) {
                        self_id_ = poll_self_id.unwrap();
                        initializer_.reset();
                    } else {
                        LOG_WARNING("UHF initialization failed. Retry.");
                        initializer_.emplace();
                    }
                }
            }

            executor_.execute(frame_service, rw_, time, rand);
            auto &&opt_res = header_receiver_.execute(rw_);
            if (opt_res.has_value()) {
                executor_.handle_response(time, etl::move(*opt_res));
            }
        }
    };
} // namespace net::link::uhf
