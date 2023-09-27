#pragma once

#include "../frame.h"
#include "./address.h"
#include <etl/optional.h>
#include <nb/stream.h>

namespace net::link::serial {
    class SendData {
        frame::BodyLength body_length_;

        nb::stream::FixedReadableBuffer<SerialAddress::SIZE + frame::BODY_LENGTH_SIZE> header_;
        nb::Promise<DataWriter> body_writer_;
        etl::optional<nb::Future<void>> barrier_;
        nb::Promise<bool> success_promise_;

      public:
        SendData() = delete;
        SendData(const SendData &) = delete;
        SendData(SendData &&) = default;
        SendData &operator=(const SendData &) = delete;
        SendData &operator=(SendData &&) = default;

        explicit SendData(
            nb::Promise<DataWriter> &&body_writer,
            nb::Promise<bool> &&success_promise,
            frame::BodyLength body_length,
            SerialAddress remote_address
        )
            : body_length_{body_length},
              header_{remote_address, body_length},
              body_writer_{etl::move(body_writer)},
              success_promise_{etl::move(success_promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (!barrier_.has_value()) {
                POLL_UNWRAP_OR_RETURN(header_.read_all_into(stream));

                auto [future, promise] = nb::make_future_promise_pair<void>();
                barrier_ = etl::move(future);
                DataWriter writer{etl::ref(stream), etl::move(promise), body_length_};
                body_writer_.set_value(etl::move(writer));
            }

            POLL_UNWRAP_OR_RETURN(barrier_.value().poll());
            success_promise_.set_value(true);
            return nb::ready();
        }
    };
} // namespace net::link::serial
