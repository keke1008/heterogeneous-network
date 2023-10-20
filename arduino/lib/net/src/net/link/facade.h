#pragma once

#include "./address.h"
#include "./detector.h"
#include "./executor.h"
#include <debug_assert.h>
#include <net/frame/service.h>
#include <util/visitor.h>

namespace net::link {
    class MediaFacade {
        etl::variant<MediaDetector, MediaExecutor> media_;
        util::Time &time_;
        util::Rand &rand_;

      public:
        MediaFacade() = delete;
        MediaFacade(const MediaFacade &) = delete;
        MediaFacade(MediaFacade &&) = default;
        MediaFacade &operator=(const MediaFacade &) = delete;
        MediaFacade &operator=(MediaFacade &&) = delete;

        inline MediaFacade(
            nb::stream::ReadableWritableStream &stream,
            util::Time &time,
            util::Rand &rand
        )
            : media_{MediaDetector{stream, time}},
              time_{time},
              rand_{rand} {}

        inline nb::Poll<void> wait_for_media_detection() {
            if (etl::holds_alternative<MediaExecutor>(media_)) {
                return nb::ready();
            }

            auto &detector = etl::get<MediaDetector>(media_);
            auto media_type = POLL_UNWRAP_OR_RETURN(detector.poll(time_));
            media_ = MediaExecutor{detector.stream(), media_type};
            return nb::ready();
        }

        inline bool is_supported_address_type(AddressType type) const {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).is_supported_address_type(type);
        }

        inline etl::optional<Address> get_address() const {
            if (etl::holds_alternative<MediaExecutor>(media_)) {
                return etl::get<MediaExecutor>(media_).get_address();
            } else {
                return etl::nullopt;
            }
        }

      public:
        inline nb::Poll<void> send_frame(SendingFrame &frame) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).send_frame(frame);
        }

        inline nb::Poll<Frame> receive_frame(frame::ProtocolNumber protocol_number) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).receive_frame(protocol_number);
        }

        template <net::frame::IFrameService FrameService>
        void execute(FrameService &service) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            auto &executor = etl::get<MediaExecutor>(media_);
            executor.execute(service, time_, rand_);
        }

        inline etl::expected<nb::Poll<nb::Future<bool>>, UnSupportedOperation>
        join_ap(etl::span<const uint8_t> ssid, etl::span<const uint8_t> password) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).join_ap(ssid, password);
        }
    };
} // namespace net::link
