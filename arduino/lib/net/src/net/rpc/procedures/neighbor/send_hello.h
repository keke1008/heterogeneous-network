#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::neighbor::send_hello {
    class AsyncParameterDeserializer {
        link::AsyncAddressDeserializer address_;
        routing::AsyncCostDeserializer cost_;

      public:
        struct Result {
            link::Address address;
            routing::Cost cost;
        };

        inline Result result() {
            return Result{.address = address_.result(), .cost = cost_.result()};
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            POLL_UNWRAP_OR_RETURN(address_.deserialize(r));
            return cost_.deserialize(r);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            link::LinkService &link_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, routing_service, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
            const auto &params = params_.result();

            POLL_UNWRAP_OR_RETURN(routing_service.poll_send_hello(params.address, params.cost));
            ctx_.set_response_property(Result::Success, 0);

            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::neighbor::send_hello
