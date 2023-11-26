#pragma once

#include "../../media.h"
#include "../common.h"
#include "./common.h"
#include <nb/barrier.h>
#include <nb/future.h>
#include <nb/poll.h>
#include <nb/serde.h>
#include <nb/time.h>
#include <net/frame/service.h>
#include <serde/hex.h>
#include <util/time.h>

namespace net::link::uhf {
    class AsyncDTCommandSerializer {
        nb::ser::AsyncStaticSpanSerializer prefix_{"@DT"};
        nb::ser::Hex<uint8_t> length_;
        frame::AsyncProtocolNumberSerializer protocol_;
        frame::AsyncFrameBufferReaderSerializer reader_;
        nb::ser::AsyncStaticSpanSerializer route_prefix_{"/R"};
        AsyncModemIdSerializer destination_;
        nb::ser::AsyncStaticSpanSerializer suffix_{"\r\n"};

      public:
        explicit AsyncDTCommandSerializer(UhfFrame &&frame)
            : length_{frame.reader.buffer_length()},
              protocol_{frame.protocol_number},
              reader_{etl::move(frame.reader)},
              destination_{frame.remote} {}

        template <nb::AsyncWritable W>
        inline nb::Poll<nb::ser::SerializeResult> serialize(W &writable) {
            POLL_UNWRAP_OR_RETURN(prefix_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(length_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(protocol_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(reader_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(route_prefix_.serialize(writable));
            POLL_UNWRAP_OR_RETURN(destination_.serialize(writable));
            return suffix_.serialize(writable);
        }

        uint8_t serialized_length() const {
            return prefix_.serialized_length() + length_.serialized_length() +
                protocol_.serialized_length() + reader_.serialized_length() +
                route_prefix_.serialized_length() + destination_.serialized_length() +
                suffix_.serialized_length();
        }
    };

    class DTExecutor {
        enum class State : uint8_t {
            CommandSending,
            CommandResponse,
            WaitingForInformationResponse,
        } state_{State::CommandSending};

        AsyncDTCommandSerializer command_;
        AsyncFixedReponseDeserializer<2> command_response_;
        AsyncFixedReponseDeserializer<2> information_response_;

        etl::optional<nb::Delay> information_response_timeout_;

      public:
        DTExecutor(UhfFrame &&frame) : command_{etl::move(frame)} {}

        template <nb::AsyncReadableWritable RW>
        nb::Poll<void> poll(RW &rw, util::Time &time) {
            if (state_ == State::CommandSending) {
                POLL_UNWRAP_OR_RETURN(command_.serialize(rw));
                state_ = State::CommandResponse;
            }

            if (state_ == State::CommandResponse) {
                POLL_UNWRAP_OR_RETURN(command_response_.deserialize(rw));

                // 説明書には6msとあるが、IRレスポンス送信終了まで待機+余裕を見て20msにしている
                auto timeout = util::Duration::from_millis(20);
                information_response_timeout_ = nb::Delay{time, timeout};
                state_ = State::WaitingForInformationResponse;
            }

            POLL_UNWRAP_OR_RETURN(information_response_timeout_->poll(time));
            information_response_.deserialize(rw);

            // 通信成功判定が欲しい場合はこれを利用する
            // bool success = information_response_.poll().is_pending();

            return nb::ready();
        }
    };
} // namespace net::link::uhf
