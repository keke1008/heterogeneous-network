#pragma once

#include "./buffer.h"
#include "./pool.h"
#include "./queue.h"
#include <etl/list.h>
#include <util/concepts.h>

namespace net::frame {
    template <typename Service, typename Address>
    concept IFrameService =
        requires(Service &service, uint8_t protocol, const Address &address, uint8_t size) {
            {
                service.request_transmission(protocol, address, size)
            } -> util::same_as<nb::Poll<FrameTransmission>>;

            {
                service.notify_reception(protocol, size)
            } -> util::same_as<nb::Poll<FrameReception<Address>>>;

            {
                service.poll_transmission_request([](const auto &request) { return false; })
            } -> util::same_as<nb::Poll<FrameTransmissionRequest<Address>>>;

            {
                service.poll_reception_notification()
            } -> util::same_as<nb::Poll<FrameReceptionNotification<Address>>>;
        };

    template <typename Address, uint8_t SHORT_BUFFER_COUNT = 8, uint8_t LARGE_BUFFER_COUNT = 4>
    class FrameService {
      public:
        static constexpr uint8_t MAX_FRAME_COUNT = SHORT_BUFFER_COUNT + LARGE_BUFFER_COUNT;

      private:
        FrameBufferAllocator<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT> allocator_;
        FrameCommunicationQueue<Address, MAX_FRAME_COUNT> queue_;

      public:
        FrameService() = default;
        FrameService(const FrameService &) = delete;
        FrameService(FrameService &&) = delete;
        FrameService &operator=(const FrameService &) = delete;
        FrameService &operator=(FrameService &&) = delete;

        nb::Poll<FrameTransmission>
        request_transmission(uint8_t protocol, const Address &destination, uint8_t size) {
            POLL_UNWRAP_OR_RETURN(queue_.wait_until_transmission_request_addable());
            auto buffer_ref = POLL_MOVE_UNWRAP_OR_RETURN(allocator_.allocate(size));
            return queue_.add_transmission_request(protocol, destination, etl::move(buffer_ref));
        }

        nb::Poll<FrameReception<Address>> notify_reception(uint8_t protocol, uint8_t size) {
            POLL_UNWRAP_OR_RETURN(queue_.wait_until_reception_notification_addable());
            auto buffer_ref = POLL_MOVE_UNWRAP_OR_RETURN(allocator_.allocate(size));
            return queue_.add_reception_notification(protocol, etl::move(buffer_ref));
        }

        // F: const FrameTransmissionRequest& -> bool
        template <typename F>
        inline nb::Poll<FrameTransmissionRequest<Address>> poll_transmission_request(F &&filter) {
            return queue_.poll_transmission_request(etl::forward<F>(filter));
        }

        nb::Poll<FrameReceptionNotification<Address>> poll_reception_notification() {
            return queue_.poll_reception_notification();
        }
    };
} // namespace net::frame
