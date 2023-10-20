#pragma once

#include "./frame.h"
#include "./procedures/link_wifi_connect_ap.h"
#include "./procedures/routing_neighbor_send_goodbye.h"
#include "./procedures/routing_neighbor_send_hello.h"
#include <util/visitor.h>

namespace net::rpc {
    template <frame::IFrameService FrameService>
    class NotImplementedProcedureExecutor {
        Request<FrameService> request_;
        ResponseHeaderWriter<FrameService> writer_{0, Result::NotImplemented};

      public:
        explicit NotImplementedProcedureExecutor(Request<FrameService> &&request)
            : request_{etl::move(request)} {}

        inline nb::Poll<void> execute() {
            auto &writer = POLL_UNWRAP_OR_RETURN(writer_.execute(request_)).get();
            return request_.send_response(etl::move(writer));
        }
    };

    template <frame::IFrameService FrameService>
    class RequestHandler {
        using Executor = etl::variant<
            routing_neighbor_send_hello::Executor<FrameService>,
            routing_neighbor_send_goodbye::Executor<FrameService>,
            link_wifi_connect_ap::Executor<FrameService>,
            NotImplementedProcedureExecutor<FrameService>>;
        Executor executor_;

      public:
        explicit RequestHandler(Request<FrameService> &&request)
            : executor_{[&]() -> Executor {
                  switch (request.procedure()) {
                  case Procedure::RoutingNeighborSendHello:
                      return routing_neighbor_send_hello::Executor<FrameService>{
                          etl::move(request)};
                  case Procedure::RoutingNeighborSendGoodbye:
                      return routing_neighbor_send_goodbye::Executor<FrameService>{
                          etl::move(request)};
                  case Procedure::LinkWifiConnectAp:
                      return link_wifi_connect_ap::Executor<FrameService>{etl::move(request)};
                  default:
                      return NotImplementedProcedureExecutor<FrameService>{etl::move(request)};
                  }
              }()} {}

        nb::Poll<void> execute(
            link::LinkService &link_service,
            routing::RoutingService &routing_service,
            util::Time &time
        ) {
            return etl::visit(
                util::Visitor{
                    [&](routing_neighbor_send_hello::Executor<FrameService> &executor) {
                        return executor.execute(link_service, routing_service, time);
                    },
                    [&](routing_neighbor_send_goodbye::Executor<FrameService> &executor) {
                        return executor.execute(link_service, routing_service, time);
                    },
                    [&](link_wifi_connect_ap::Executor<FrameService> &executor) {
                        return executor.execute(link_service, time);
                    },
                    [&](NotImplementedProcedureExecutor<FrameService> &executor) {
                        return executor.execute();
                    },
                },
                executor_
            );
        }
    };
} // namespace net::rpc
