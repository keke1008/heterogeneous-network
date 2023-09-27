#pragma once

#include "./detector.h"
#include "./executor.h"
#include <debug_assert.h>
#include <util/visitor.h>

namespace net::link {
    class MediaFacade {
        etl::variant<MediaDetector, MediaExecutor> media_;

      public:
        MediaFacade() = delete;
        MediaFacade(const MediaFacade &) = delete;
        MediaFacade(MediaFacade &&) = default;
        MediaFacade &operator=(const MediaFacade &) = default;
        MediaFacade &operator=(MediaFacade &&) = delete;

        inline MediaFacade(nb::stream::ReadableWritableStream &stream, util::Time &time)
            : media_{MediaDetector{stream, time}} {}

        inline nb::Poll<void> wait_for_media_detection(util::Time &time) {
            if (etl::holds_alternative<MediaExecutor>(media_)) {
                return nb::ready();
            }

            auto &detector = etl::get<MediaDetector>(media_);
            auto media_type = POLL_UNWRAP_OR_RETURN(detector.poll(time));
            media_ = MediaExecutor{detector.stream(), media_type};
            return nb::ready();
        }

        inline bool is_supported_address_type(AddressType type) const {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).is_supported_address_type(type);
        }

        nb::Poll<FrameTransmission> inline send_frame(
            const Address &address,
            const frame::BodyLength length
        ) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).send_frame(address, length);
        }

        inline nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return etl::get<MediaExecutor>(media_).execute(time, rand);
        }
    };
} // namespace net::link
