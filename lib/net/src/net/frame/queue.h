#pragma once

#include "./buffer.h"
#include <etl/list.h>
#include <nb/future.h>

namespace net::frame {
    template <typename Address>
    struct FrameTransmissionRequest {
        uint8_t protocol;
        Address destination;
        FrameBufferReader reader;
        nb::Promise<bool> success;
    };

    template <typename Address>
    struct FrameReceptionNotification {
        uint8_t protocol;
        FrameBufferReader reader;
        nb::Future<Address> source;
    };

    struct FrameTransmission {
        FrameBufferWriter writer;
        nb::Future<bool> success;
    };

    template <typename Address>
    struct FrameReception {
        FrameBufferWriter writer;
        nb::Promise<Address> source;
    };

    template <typename Address, uint8_t MAX_FRAME_COUNT>
    class FrameCommunicationQueue {
        etl::list<FrameTransmissionRequest<Address>, MAX_FRAME_COUNT> transmission_requests_;
        etl::list<FrameReceptionNotification<Address>, MAX_FRAME_COUNT> reception_notifications_;

      public:
        FrameCommunicationQueue() = default;
        FrameCommunicationQueue(const FrameCommunicationQueue &) = delete;
        FrameCommunicationQueue(FrameCommunicationQueue &&) = delete;
        FrameCommunicationQueue &operator=(const FrameCommunicationQueue &) = delete;
        FrameCommunicationQueue &operator=(FrameCommunicationQueue &&) = delete;

        nb::Poll<void> wait_until_transmission_request_addable() {
            return transmission_requests_.full() ? nb::pending : nb::ready();
        }

        nb::Poll<void> wait_until_reception_notification_addable() {
            return reception_notifications_.full() ? nb::pending : nb::ready();
        }

        nb::Poll<FrameTransmission> add_transmission_request(
            uint8_t protocol,
            const Address &destination,
            FrameBufferReference &&buffer_ref
        ) {
            if (transmission_requests_.full()) {
                return nb::pending;
            }
            auto [reader, writer] = make_frame_buffer_pair(etl::move(buffer_ref));
            auto [future, promise] = nb::make_future_promise_pair<bool>();
            transmission_requests_.push_back(FrameTransmissionRequest<Address>{
                protocol,
                destination,
                etl::move(reader),
                etl::move(promise),
            });
            return FrameTransmission{etl::move(writer), etl::move(future)};
        }

        nb::Poll<FrameReception<Address>>
        add_reception_notification(uint8_t protocol, FrameBufferReference &&buffer_ref) {
            if (reception_notifications_.full()) {
                return nb::pending;
            }
            auto [reader, writer] = make_frame_buffer_pair(etl::move(buffer_ref));
            auto [future, promise] = nb::make_future_promise_pair<Address>();
            reception_notifications_.push_back(FrameReceptionNotification<Address>{
                protocol,
                etl::move(reader),
                etl::move(future),
            });
            return FrameReception<Address>{etl::move(writer), etl::move(promise)};
        }

        // F: const FrameTransmissionRequest& -> bool
        template <typename F>
        nb::Poll<FrameTransmissionRequest<Address>> poll_transmission_request(F &&filter) {
            auto it = transmission_requests_.begin();
            while (it != transmission_requests_.end()) {
                if (filter(*it)) {
                    auto found = etl::move(*it);
                    transmission_requests_.erase(it);
                    return etl::move(found);
                }
                ++it;
            }
            return nb::pending;
        }

        // F: const FrameReceptionNotification& -> bool
        template <typename F>
        nb::Poll<FrameReceptionNotification<Address>> poll_reception_notification(F &&filter) {
            auto it = reception_notifications_.begin();
            while (it != reception_notifications_.end()) {
                if (filter(*it)) {
                    auto found = etl::move(*it);
                    reception_notifications_.erase(it);
                    return etl::move(found);
                }
                ++it;
            }
            return nb::pending;
        }
    };
}; // namespace net::frame
