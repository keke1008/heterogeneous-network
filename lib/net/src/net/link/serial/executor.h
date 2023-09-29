#pragma once

#include "./address.h"
#include "./receive_data.h"
#include "./send_data.h"
#include <debug_assert.h>
#include <etl/optional.h>
#include <net/frame/service.h>

namespace net::link::serial {
    class SerialExecutor {
        nb::stream::ReadableWritableStream &stream_;

        SerialAddress address_;
        etl::optional<SendData> send_data_;
        ReceiveData receive_data_;

      public:
        SerialExecutor() = delete;
        SerialExecutor(const SerialExecutor &) = delete;
        SerialExecutor(SerialExecutor &&) = default;
        SerialExecutor &operator=(const SerialExecutor &) = delete;
        SerialExecutor &operator=(SerialExecutor &&) = delete;

        explicit SerialExecutor(nb::stream::ReadableWritableStream &stream, SerialAddress address)
            : stream_{stream},
              address_{address} {}

        inline void set_address(SerialAddress address) {
            address_ = address;
        }

        inline bool is_supported_address_type(AddressType type) const {
            return type == AddressType::Serial;
        }

      public:
        template <net::frame::IFrameService<Address> Service>
        void execute(Service &service) {
            if (send_data_.has_value()) {
                auto poll = send_data_.value().execute(stream_);
                if (poll.is_ready()) {
                    send_data_ = etl::nullopt;
                }
            }

            if (!send_data_.has_value()) {
                auto poll_request = service.poll_transmission_request([](auto &request) {
                    return request.destination.type() == AddressType::Serial;
                });
                if (poll_request.is_ready()) {
                    send_data_ = SendData{etl::move(poll_request.unwrap()), address_};
                    send_data_.value().execute(stream_);
                }
            }

            auto poll = receive_data_.execute(service, stream_);
            if (poll.is_ready()) {
                receive_data_ = ReceiveData{};
                receive_data_.execute(service, stream_);
            }
        }
    };
} // namespace net::link::serial
