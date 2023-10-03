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

      public:
        MediaFacade() = delete;
        MediaFacade(const MediaFacade &) = delete;
        MediaFacade(MediaFacade &&) = default;
        MediaFacade &operator=(const MediaFacade &) = delete;
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

      public:
        template <net::frame::IFrameService FrameService>
        void execute(FrameService &service, util::Time &time, util::Rand &rand) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            auto &executor = etl::get<MediaExecutor>(media_);
            executor.execute(service, time, rand);
        }
    };
} // namespace net::link
