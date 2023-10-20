#pragma once

#include "../frame.h"

namespace net::rpc::routing_neighbor_send_goodbye {
    template <frame::IFrameService FrameService>
    class Executor {
        Request<FrameService> request_;
        routing::AsyncNodeIdParser peer_parser_;
        etl::optional<ResponseHeaderWriter<FrameService>> response_;

      public:
        explicit Executor(Request<FrameService> &&request) : request_{etl::move(request)} {}

        nb::Poll<void> execute(
            link::LinkService &link_service,
            routing::RoutingService &routing_service,
            util::Time &time
        ) {
            if (!response_.has_value()) {
                if (request_.is_timeout(time)) {
                    response_.emplace(0, Result::Busy);
                    return nb::pending;
                }

                POLL_UNWRAP_OR_RETURN(request_.body().read(peer_parser_));
                const auto &peer = peer_parser_.result();
                POLL_UNWRAP_OR_RETURN(routing_service.request_goodbye(peer));
                response_.emplace(0, Result::Success);
            }

            auto &writer = POLL_UNWRAP_OR_RETURN(response_.execute(request_)).get();
            return request_.send_response(etl::move(writer));
        }
    };
}; // namespace net::rpc::routing_neighbor_send_goodbye
