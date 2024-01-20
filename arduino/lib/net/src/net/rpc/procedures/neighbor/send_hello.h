#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::neighbor::send_hello {
    class AsyncParameterDeserializer {
        link::AsyncAddressDeserializer address_;
        node::AsyncCostDeserializer cost_;
        nb::de::Optional<link::AsyncMediaPortNumberDeserializer> media_port_;

      public:
        struct Result {
            link::Address address;
            node::Cost cost;
            etl::optional<link::MediaPortNumber> media_port;
        };

        inline Result result() {
            return Result{
                .address = address_.result(),
                .cost = cost_.result(),
                .media_port = media_port_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            POLL_UNWRAP_OR_RETURN(address_.deserialize(r));
            POLL_UNWRAP_OR_RETURN(cost_.deserialize(r));
            return media_port_.deserialize(r);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer params_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            link::LinkService &link_service,
            const net::local::LocalNodeService &local_node_service,
            net::neighbor::NeighborService &neighbor_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
            const auto &params = params_.result();

            POLL_UNWRAP_OR_RETURN(neighbor_service.poll_send_hello(
                link_service, local_node_service, params.address, params.cost, params.media_port
            ));
            ctx_.set_response_property(Result::Success, 0);

            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::neighbor::send_hello
