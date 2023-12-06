#pragma once

#include "../../request.h"
#include <nb/serde.h>

namespace net::rpc::neighbor::send_goodbye {
    class AsyncParameterDeserializer {
        node::AsyncNodeIdDeserializer node_id_;

      public:
        struct Result {
            node::NodeId node_id;
        };

        inline Result result() {
            return Result{.node_id = node_id_.result()};
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            return node_id_.deserialize(r);
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
            const node::LocalNodeService &local_node_service,
            net::neighbor::NeighborService<RW> &neighbor_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
            const auto &params = params_.result();

            POLL_UNWRAP_OR_RETURN(
                neighbor_service.poll_send_goodbye(local_node_service, params.node_id)
            );
            ctx_.set_response_property(Result::Success, 0);

            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::neighbor::send_goodbye
