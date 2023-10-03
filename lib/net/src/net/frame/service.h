#pragma once

#include "./buffer.h"
#include "./pool.h"
#include "./queue.h"
#include <etl/list.h>
#include <util/concepts.h>

namespace net::frame {
    template <typename Service>
    concept IFrameService = requires(
        Service &service,
        uint8_t protocol,
        Service::Address &address,
        uint8_t size,
        FrameBufferReader reader
    ) {
        {
            service.request_transmission(protocol, address, size)
        } -> util::same_as<nb::Poll<FrameTransmission>>;

        {
            service.request_transmission(protocol, address, etl::move(reader))
        } -> util::same_as<nb::Poll<nb::Future<bool>>>;

        {
            service.notify_reception(protocol, size)
        } -> util::same_as<nb::Poll<FrameReception<typename Service::Address>>>;

        {
            service.poll_transmission_request([](const auto &request) { return false; })
        } -> util::same_as<nb::Poll<FrameTransmissionRequest<typename Service::Address>>>;

        {
            service.poll_reception_notification([](const auto &notification) { return false; })
        } -> util::same_as<nb::Poll<FrameReceptionNotification<typename Service::Address>>>;

        {
            service.poll_reception_notification(protocol)
        } -> util::same_as<nb::Poll<FrameReceptionNotification<typename Service::Address>>>;
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

        nb::Poll<nb::Future<bool>> request_transmission(
            uint8_t protocol,
            const Address &destination,
            FrameBufferReader &&reader
        ) {
            return queue_.add_transmission_request(protocol, destination, etl::move(reader));
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

        // F: const FrameReceptionNotification& -> bool
        template <typename F>
        inline nb::Poll<FrameReceptionNotification<Address>> poll_reception_notification(F &&filter
        ) {
            return queue_.poll_reception_notification(etl::forward<F>(filter));
        }

        inline nb::Poll<FrameReceptionNotification<Address>>
        poll_reception_notification(uint8_t protocol) {
            return queue_.poll_reception_notification(
                [protocol](const FrameReceptionNotification<Address> &notification) {
                    return notification.protocol == protocol;
                }
            );
        }
    };
} // namespace net::frame
