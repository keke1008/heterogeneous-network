#pragma once

#include "./frame.h"
#include "./procedure.h"
#include <net/routing/socket.h>

namespace net::rpc {
    template <frame::IFrameService FrameService>
    class RpcService {
        routing::Acceptor<FrameService> acceptor_{frame::ProtocolNumber::Rpc};
        etl::variant<etl::monostate, RequestReceiver<FrameService>, RequestHandler<FrameService>>
            handler_;

      public:
        void execute(
            FrameService &frame_service,
            link::LinkService &link_service,
            routing::RoutingService &routing_service,
            util::Time &time
        ) {
            if (etl::holds_alternative<etl::monostate>(handler_)) {
                auto poll_socket = acceptor_.accept();
                if (poll_socket.is_pending()) {
                    return;
                }
                handler_.template emplace<RequestReceiver<FrameService>>(
                    etl::move(poll_socket.unwrap())
                );
            }

            if (etl::holds_alternative<RequestReceiver<FrameService>>(handler_)) {
                auto &receiver = etl::get<RequestReceiver<FrameService>>(handler_);
                auto poll_request = receiver.execute(time);
                if (poll_request.is_pending()) {
                    return;
                }
                handler_.template emplace<RequestHandler<FrameService>>(
                    etl::move(poll_request.unwrap())
                );
            }

            if (etl::holds_alternative<RequestHandler<FrameService>>(handler_)) {
                auto &handler = etl::get<RequestHandler<FrameService>>(handler_);
                if (handler.execute().is_pending()) {
                    return;
                }
                handler_.template emplace<etl::monostate>();
            }
        }
    };
}; // namespace net::rpc
