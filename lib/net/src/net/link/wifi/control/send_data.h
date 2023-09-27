#pragma once

#include "../address.h"
#include "../response.h"
#include <etl/optional.h>
#include <nb/buf.h>
#include <nb/poll.h>
#include <nb/stream.h>
#include <net/link/frame.h>

namespace net::link::wifi {
    class SendData {
        uint8_t length_;

        nb::stream::FixedReadableBuffer<40> prefix_;
        nb::stream::DiscardingUntilByteWritableBuffer body_prompt_{'>'};

        nb::Promise<DataWriter> body_promise_;
        etl::optional<nb::Future<void>> barrier_;

        // "SEND OK\r\n" or "SEND FAIL\r\n" のため11バイト必要
        nb::stream::MaxLengthSingleLineWrtableBuffer<11> response_;

        nb::Promise<bool> result_promise_;

      public:
        explicit SendData(
            nb::Promise<DataWriter> &&body_promise,
            nb::Promise<bool> &&result_promise,
            uint8_t length,
            IPv4Address &remote_address,
            uint16_t remote_port
        )
            : length_{length},
              prefix_{
                  R"(AT+CIPSEND=)", // フォーマッタ抑制用コメント
                  nb::buf::FormatDecimal(length),
                  R"(,")",
                  remote_address,
                  R"(",)",
                  nb::buf::FormatDecimal(remote_port),
                  "\r\n",
              },
              body_promise_{etl::move(body_promise)},
              result_promise_{etl::move(result_promise)} {}

        nb::Poll<void> execute(nb::stream::ReadableWritableStream &stream) {
            if (!barrier_.has_value()) {
                POLL_UNWRAP_OR_RETURN(prefix_.read_all_into(stream));
                POLL_UNWRAP_OR_RETURN(body_prompt_.write_all_from(stream));

                auto [future, promise] = nb::make_future_promise_pair<void>();
                barrier_ = etl::move(future);
                DataWriter writer{etl::ref(stream), etl::move(promise), length_};
                body_promise_.set_value(etl::move(writer));
            }

            POLL_UNWRAP_OR_RETURN(barrier_.value().poll());

            while (true) {
                POLL_UNWRAP_OR_RETURN(response_.write_all_from(stream));
                auto line = POLL_UNWRAP_OR_RETURN(response_.poll());

                if (Response<ResponseType::SEND_OK>::try_parse(line)) {
                    result_promise_.set_value(true);
                    return nb::ready();
                }

                if (Response<ResponseType::SEND_FAIL>::try_parse(line)) {
                    result_promise_.set_value(false);
                    return nb::ready();
                }

                response_.reset();
            }
        }
    };
} // namespace net::link::wifi
