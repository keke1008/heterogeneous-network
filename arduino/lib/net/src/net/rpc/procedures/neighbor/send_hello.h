#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::neighbor::send_hello {
    class AsyncParameterDeserializer {
        link::AsyncAddressDeserializer address_;
        node::AsyncCostDeserializer cost_;

      public:
        struct Result {
            link::Address address;
            node::Cost cost;
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

    template <nb::AsyncReadableWritable RW>
    class Executor {
        RequestContext<RW> ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext<RW> &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            link::LinkService<RW> &link_service,
            node::LocalNodeService &local_node_service,
            routing::RoutingService<RW> &routing_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
            const auto &params = params_.result();

            POLL_UNWRAP_OR_RETURN(
                routing_service.poll_send_hello(local_node_service, params.address, params.cost)
            );
            ctx_.set_response_property(Result::Success, 0);

            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::neighbor::send_hello
