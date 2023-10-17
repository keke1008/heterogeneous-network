#pragma once

#include "../../media.h"
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
        frame::FrameBufferReader &&reader_;
        nb::stream::FixedReadableBuffer<6> prefix_length_protocol_;
        nb::stream::FixedReadableBuffer<6> route_suffix_;

      public:
        explicit DTCommand(SendingFrame &frame)
            : reader_{etl::move(frame.reader_ref)},
              prefix_length_protocol_{
                  "@DT",
                  nb::buf::FormatHexaDecimal<uint8_t>(
                      reader_.frame_length() + frame::PROTOCOL_SIZE
                  ),
                  frame::ProtocolNumberWriter(frame.protocol_number),
              },
              route_suffix_{"/R", ModemId(frame.peer).span(), "\r\n"} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream) {
            POLL_UNWRAP_OR_RETURN(prefix_length_protocol_.read_all_into(stream));
            POLL_UNWRAP_OR_RETURN(reader_.read_all_into(stream));
            return route_suffix_.read_all_into(stream);
        }
    };

    class DTExecutor {
        enum class State : uint8_t {
            CommandSending,
            CommandResponse,
            WaitingForInformationResponse,
        } state_{State::CommandSending};

        DTCommand command_;
        FixedResponseWriter<2> command_response_;
        FixedResponseWriter<2> information_response_;

        etl::optional<nb::Delay> information_response_timeout_;

      public:
        DTExecutor(SendingFrame &frame) : command_{frame} {}

        nb::Poll<void> poll(nb::stream::ReadableWritableStream &stream, util::Time &time) {
            if (state_ == State::CommandSending) {
                POLL_UNWRAP_OR_RETURN(command_.poll(stream));
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

            // 通信成功判定が欲しい場合はこれを利用する
            // bool success = information_response_.poll().is_pending();

            return nb::ready();
        }
    };
} // namespace net::link::uhf
