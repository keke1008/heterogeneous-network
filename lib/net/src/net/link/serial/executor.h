#pragma once

#include "./address.h"
#include "./receive_data.h"
#include "./send_data.h"
#include <debug_assert.h>
#include <etl/optional.h>

namespace net::link::serial {
    class SerialExecutor {
        nb::stream::ReadableWritableStream &stream_;

        SerialAddress address_;
        etl::optional<SendData> send_data_;
        etl::optional<ReceiveData> receive_data_;

      public:
        explicit SerialExecutor(nb::stream::ReadableWritableStream &stream, SerialAddress address)
            : stream_{stream},
              address_{address} {}

        inline void set_address(SerialAddress address) {
            address_ = address;
        }

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::Serial;
        }

        nb::Poll<FrameTransmission>
        send_data(const Address &destination, frame::BodyLength body_length) {
            DEBUG_ASSERT(destination.type() == AddressType::Serial);
            if (send_data_.has_value()) {
                return nb::pending;
            }

            auto [frame, p_body, p_success] = FrameTransmission::make_frame_transmission();
            SerialAddress address{destination};
            send_data_ = SendData{etl::move(p_body), etl::move(p_success), body_length, address};
            return nb::ready(etl::move(frame));
        }

      public:
        nb::Poll<FrameReception> execute() {
            if (send_data_.has_value()) {
                auto poll = send_data_.value().execute(stream_);
                if (poll.is_ready()) {
                    send_data_ = etl::nullopt;
                }
            }

            if (receive_data_.has_value()) {
                auto poll = receive_data_.value().execute(stream_);
                if (poll.is_ready()) {
                    receive_data_ = etl::nullopt;
                }
            }

            if (!receive_data_.has_value() && stream_.readable_count() > 0) {
                auto [body_future, body_promise] = nb::make_future_promise_pair<DataReader>();
                auto [source_future, source_promise] = nb::make_future_promise_pair<Address>();
                receive_data_ = ReceiveData{etl::move(body_promise), etl::move(source_promise)};
                receive_data_.value().execute(stream_);
                return nb::ready(FrameReception{
                    .body = etl::move(body_future),
                    .source = etl::move(source_future),
                });
            }

            return nb::pending;
        }
    };
} // namespace net::link::serial
