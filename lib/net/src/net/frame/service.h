#pragma once

#include "./pool.h"
#include <etl/list.h>
#include <nb/future.h>
#include <util/concepts.h>

namespace net::frame {
    template <typename Address>
    struct FrameTransmissionRequest {
        Address destination;
        FrameBufferReader reader;
        nb::Promise<bool> success;
    };

    template <typename Address>
    struct FrameReceptionNotification {
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

    template <typename Service, typename Address>
    concept IFrameService = requires(Service &service, const Address &address, uint8_t size) {
        {
            service.request_transmission(address, size)
        } -> util::same_as<nb::Poll<FrameTransmission>>;

        { service.notify_reception(size) } -> util::same_as<nb::Poll<FrameReception<Address>>>;

        {
            service.poll_transmission_request([](const auto &request) { return false; })
        } -> util::same_as<nb::Poll<FrameTransmissionRequest<Address>>>;

        {
            service.poll_reception_notification()
        } -> util::same_as<nb::Poll<FrameReceptionNotification<Address>>>;
    };

    template <typename Address, uint8_t SHORT_BUFFER_COUNT = 12, uint8_t LARGE_BUFFER_COUNT = 6>
    class FrameService {
      public:
        static constexpr uint8_t MAX_FRAME_COUNT = SHORT_BUFFER_COUNT + LARGE_BUFFER_COUNT;

      private:
        FrameBufferAllocator<SHORT_BUFFER_COUNT, LARGE_BUFFER_COUNT> allocator_;
        etl::list<FrameTransmissionRequest<Address>, MAX_FRAME_COUNT / 2> transmission_requests_;
        etl::list<FrameReceptionNotification<Address>, MAX_FRAME_COUNT / 2>
            reception_notifications_;

      public:
        FrameService() = default;
        FrameService(const FrameService &) = delete;
        FrameService(FrameService &&) = delete;
        FrameService &operator=(const FrameService &) = delete;
        FrameService &operator=(FrameService &&) = delete;

        nb::Poll<FrameTransmission> request_transmission(const Address &destination, uint8_t size) {
            if (transmission_requests_.full()) {
                return nb::pending;
            }
            auto [reader, writer] = POLL_UNWRAP_OR_RETURN(allocator_.allocate(size));
            auto [future, promise] = nb::make_future_promise_pair<bool>();
            transmission_requests_.push_back(FrameTransmissionRequest<Address>{
                destination,
                etl::move(reader),
                etl::move(promise),
            });
            return FrameTransmission{etl::move(writer), etl::move(future)};
        }

        nb::Poll<FrameReception<Address>> notify_reception(uint8_t size) {
            if (reception_notifications_.full()) {
                return nb::pending;
            }
            auto [reader, writer] = POLL_UNWRAP_OR_RETURN(allocator_.allocate(size));
            auto [future, promise] = nb::make_future_promise_pair<Address>();
            reception_notifications_.push_back(FrameReceptionNotification<Address>{
                etl::move(reader),
                etl::move(future),
            });
            return FrameReception<Address>{etl::move(writer), etl::move(promise)};
        }

        // F: const FrameTransmissionRequest& -> bool
        template <typename F>
        inline nb::Poll<FrameTransmissionRequest<Address>> poll_transmission_request(F &&filter) {
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

        nb::Poll<FrameReceptionNotification<Address>> poll_reception_notification() {
            if (reception_notifications_.empty()) {
                return nb::pending;
            }

            auto found = etl::move(reception_notifications_.front());
            reception_notifications_.pop_front();
            return etl::move(found);
        }
    };
} // namespace net::frame
