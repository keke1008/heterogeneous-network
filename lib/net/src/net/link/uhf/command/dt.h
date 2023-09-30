#pragma once

#include "../common.h"
#include "./common.h"
#include <nb/barrier.h>
#include <nb/buf.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <nb/time.h>
#include <net/frame/service.h>
#include <serde/hex.h>
#include <util/time.h>

namespace net::link::uhf {
    class DTCommand {
        enum class State : uint8_t {
            PrefixLengthProtocol,
            Body,
            RouteSuffix,
        } state_{State::PrefixLengthProtocol};

        nb::stream::FixedReadableBuffer<6> prefix_length_protocol_;
        nb::stream::FixedReadableBuffer<6> route_suffix_;

      public:
        explicit DTCommand(net::frame::FrameTransmissionRequest<Address> &body)
            : prefix_length_protocol_{
                  "@DT",
                  nb::buf::FormatHexaDecimal<uint8_t>(body.reader.frame_length() + frame::PROTOCOL_SIZE),
                  nb::buf::FormatBinary(body.protocol),
              },
              route_suffix_{"/R", ModemId(body.destination).span(), "\r\n"} {}

        nb::Poll<void> poll(
            nb::stream::ReadableWritableStream &stream,
            net::frame::FrameTransmissionRequest<Address> &body
        ) {
            if (state_ == State::PrefixLengthProtocol) {
                POLL_UNWRAP_OR_RETURN(prefix_length_protocol_.read_all_into(stream));
                state_ = State::Body;
            }

            if (state_ == State::Body) {
                POLL_UNWRAP_OR_RETURN(body.reader.read_all_into(stream));
                state_ = State::RouteSuffix;
            }

            return route_suffix_.read_all_into(stream);
        }
    };

    class DTExecutor {
        enum class State : uint8_t {
            CommandSending,
            CommandResponse,
            WaitingForInformationResponse,
        } state_{State::CommandSending};

        net::frame::FrameTransmissionRequest<Address> request_;
        DTCommand command_;
        FixedResponseWriter<2> command_response_;
        FixedResponseWriter<2> information_response_;

        etl::optional<nb::Delay> information_response_timeout_;

      public:
        DTExecutor(net::frame::FrameTransmissionRequest<Address> &&request)
            : request_{etl::move(request)},
              command_{request_} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream, util::Time &time) {
            if (state_ == State::CommandSending) {
                POLL_UNWRAP_OR_RETURN(command_.poll(stream, request_));
                state_ = State::CommandResponse;
            }

            if (state_ == State::CommandResponse) {
                POLL_UNWRAP_OR_RETURN(command_response_.write_all_from(stream));

                // 説明書には6msとあるが、IRレスポンス送信終了まで待機+余裕を見て20msにしている
                auto timeout = util::Duration::from_millis(20);
                information_response_timeout_ = nb::Delay{time, timeout};
                state_ = State::WaitingForInformationResponse;
            }

            POLL_UNWRAP_OR_RETURN(information_response_timeout_->poll(time));
            information_response_.write_all_from(stream);

            bool success = information_response_.poll().is_pending();
            request_.success.set_value(success);
            return nb::ready();
        }
    };
} // namespace net::link::uhf
