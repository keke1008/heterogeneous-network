#pragma once

#include "./data.h"
#include "./layout.h"
#include <etl/circular_buffer.h>
#include <etl/optional.h>

namespace net::link {
    struct FrameTransmission {
        DataWriter body;
        nb::Future<bool> success;
    };

    struct FrameTransmissionRequestParameters {
        Address destination;
        frame::BodyLength length;
    };

    struct FrameTransmissionRequest {
        nb::Promise<FrameTransmission> promise;
        FrameTransmissionRequestParameters parameters;
    };

    struct FrameTransmissionTask {
        nb::Promise<FrameTransmission> promise;
        FrameTransmissionFuture future;

        FrameTransmissionTask() = delete;
        FrameTransmissionTask(const FrameTransmissionTask &) = delete;
        FrameTransmissionTask(FrameTransmissionTask &&) = default;
        FrameTransmissionTask &operator=(const FrameTransmissionTask &) = delete;
        FrameTransmissionTask &operator=(FrameTransmissionTask &&) = default;

        inline FrameTransmissionTask(
            nb::Promise<FrameTransmission> &&promise,
            FrameTransmissionFuture &&future
        )
            : promise{etl::move(promise)},
              future{etl::move(future)} {}

        nb::Poll<void> execute() {
            auto &body_future = future.body;

            POLL_UNWRAP_OR_RETURN(body_future.poll());
            auto body = body_future.poll().unwrap();
            promise.set_value(FrameTransmission{
                etl::move(body.get()),
                etl::move(future.success),
            });
            return nb::ready();
        }
    };

    class FrameTransmissionBuffer {
        static constexpr uint8_t BUFFER_SIZE = 16;

        etl::circular_buffer<FrameTransmissionRequest, BUFFER_SIZE> requests_;
        etl::optional<FrameTransmissionTask> current_task_;

      public:
        FrameTransmissionBuffer() = default;
        FrameTransmissionBuffer(const FrameTransmissionBuffer &) = delete;
        FrameTransmissionBuffer(FrameTransmissionBuffer &&) = default;
        FrameTransmissionBuffer &operator=(const FrameTransmissionBuffer &) = delete;
        FrameTransmissionBuffer &operator=(FrameTransmissionBuffer &&) = default;

        nb::Future<FrameTransmission> add_request(Address destination, frame::BodyLength length) {
            auto [future, promise] = nb::make_future_promise_pair<FrameTransmission>();
            requests_.push(FrameTransmissionRequest{
                etl::move(promise),
                etl::move(FrameTransmissionRequestParameters{destination, length}),
            });
            return etl::move(future);
        }

        nb::Poll<etl::reference_wrapper<FrameTransmissionRequestParameters>>
        get_request_parameters() {
            if (requests_.empty()) {
                return nb::pending;
            }
            auto &request = requests_.front();
            return etl::ref(request.parameters);
        }

        void start_transmission(FrameTransmissionFuture &&future) {
            DEBUG_ASSERT(!current_task_.has_value());
            auto &request = requests_.front();
            current_task_.emplace(etl::move(request.promise), etl::move(future));
            requests_.pop();
        }

        void execute() {
            if (!current_task_.has_value()) {
                return;
            }

            auto &task = current_task_.value();
            if (task.execute().is_ready()) {
                current_task_.reset();
            }
        }
    };

    struct FrameReception {
        DataReader body;
        nb::Future<Address> source;
    };

    class FrameReceptionBuffer {
        etl::optional<FrameReceptionFuture> current_future_;

      public:
        FrameReceptionBuffer() = default;
        FrameReceptionBuffer(const FrameReceptionBuffer &) = delete;
        FrameReceptionBuffer(FrameReceptionBuffer &&) = default;
        FrameReceptionBuffer &operator=(const FrameReceptionBuffer &) = delete;
        FrameReceptionBuffer &operator=(FrameReceptionBuffer &&) = default;

        void start_reception(FrameReceptionFuture &&future) {
            DEBUG_ASSERT(!current_future_.has_value());
            current_future_.emplace(etl::move(future));
        }

        nb::Poll<FrameReception> execute() {
            if (!current_future_.has_value()) {
                return nb::pending;
            }

            auto &future = current_future_.value();
            auto &body_future = future.body;
            POLL_UNWRAP_OR_RETURN(body_future.poll());

            auto body = body_future.poll().unwrap();
            current_future_.reset();
            return nb::ready(FrameReception{
                etl::move(body.get()),
                etl::move(future.source),
            });
        }
    };
} // namespace net::link
