#pragma once

#include "../frame.h"

namespace net::rpc::link_wifi_connect_ap {
    class AsyncStringWithLengthPrefixParser {
        nb::buf::AsyncBinParsre<uint8_t> length_parser_;
        etl::optional<etl::span<const uint8_t>> result_;

      public:
        template <nb::buf::IAsyncBuffer AsyncBuffer>
        inline nb::Poll<void> parse(nb::buf::AsyncBufferSplitter<AsyncBuffer> &splitter) {
            POLL_UNWRAP_OR_RETURN(length_parser_.parse(splitter));
            result_ = splitter.split_nbytes(length_parser_.result());
            return nb::ready();
        }

        inline etl::span<const uint8_t> result() {
            return result_.value();
        }
    };

    template <frame::IFrameService FrameService>
    class Executor {
        enum class State : uint8_t {
            Parsing,
            Executing,
            Responding,
        } state_{State::Parsing};

        Request<FrameService> request_;
        AsyncStringWithLengthPrefixParser ssid_parser_;
        AsyncStringWithLengthPrefixParser password_parser_;

        etl::optional<nb::Future<bool>> success_;
        etl::optional<ResponseHeaderWriter<FrameService>> response_;

      public:
        explicit Executor(Request<FrameService> &&request) : request_{etl::move(request)} {}

        nb::Poll<void> execute(link::LinkService &link_service, util::Time &time) {
            if (state_ == State::Parsing) {
                if (request_.is_timeout(time)) {
                    response_.emplace(0, Result::Busy);
                    state_ = State::Responding;
                    return nb::pending;
                }

                POLL_UNWRAP_OR_RETURN(ssid_parser_.parse(request_.body()));
                POLL_UNWRAP_OR_RETURN(password_parser_.parse(request_.body()));

                auto ssid = ssid_parser_.result();
                auto password = password_parser_.result();
                auto result = link_service.join_ap(ssid, password);
                if (result.has_value()) {
                    success_ = POLL_MOVE_UNWRAP_OR_RETURN(etl::move(result.value()));
                    state_ = State::Executing;
                } else {
                    response_.emplace(0, Result::NotSupported);
                    state_ = State::Responding;
                }
            }

            if (state_ == State::Executing) {
                auto &future = success_.value();
                bool is_ready = POLL_UNWRAP_OR_RETURN(future.poll()).get();
                if (is_ready) {
                    response_.emplace(0, Result::Success);
                } else {
                    response_.emplace(0, Result::Failed);
                }
            }

            if (state_ == State::Responding) {
                auto &writer = POLL_UNWRAP_OR_RETURN(response_.execute(request_)).get();
                return request_.send_response(etl::move(writer));
            }

            return nb::pending;
        }
    };
}; // namespace net::rpc::link_wifi_connect_ap
