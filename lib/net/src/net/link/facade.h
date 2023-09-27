#pragma once

#include "./detector.h"
#include "./executor.h"
#include "./frame.h"
#include <debug_assert.h>
#include <util/visitor.h>

namespace net::link {
    class MediaFacade {
        etl::variant<MediaDetector, MediaExecutor> media_;
        FrameTransmissionBuffer transmission_buffer_;
        FrameReceptionBuffer reception_buffer_;

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

        nb::Future<FrameTransmission> inline send_frame(
            const Address &address,
            const frame::BodyLength length
        ) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            return transmission_buffer_.add_request(address, length);
        }

      private:
        inline nb::Poll<void> execute_transmission(MediaExecutor &executor) {
            auto parameters = POLL_UNWRAP_OR_RETURN(transmission_buffer_.get_request_parameters());
            auto &p = parameters.get();
            auto future = POLL_MOVE_UNWRAP_OR_RETURN(executor.send_frame(p.destination, p.length));
            transmission_buffer_.start_transmission(etl::move(future));

            transmission_buffer_.execute();
            return nb::ready();
        }

      public:
        inline nb::Poll<FrameReception> execute(util::Time &time, util::Rand &rand) {
            DEBUG_ASSERT(etl::holds_alternative<MediaExecutor>(media_));
            auto &executor = etl::get<MediaExecutor>(media_);

            execute_transmission(executor);

            auto future = POLL_MOVE_UNWRAP_OR_RETURN(executor.execute(time, rand));
            reception_buffer_.start_reception(etl::move(future));
            return reception_buffer_.execute();
        }
    };
} // namespace net::link
