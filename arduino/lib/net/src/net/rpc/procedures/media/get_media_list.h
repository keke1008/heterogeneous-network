#pragma once

#include "../../request.h"
#include <nb/serde.h>
#include <net/link.h>

namespace net::rpc::media::get_media_list {
    using AsyncResultSerializer =
        nb::ser::Vec<nb::ser::Optional<link::AsyncMediaInfoSerializer>, link::MAX_MEDIA_PORT>;

    class Executor {
        RequestContext ctx_;
        etl::optional<AsyncResultSerializer> result_;

      public:
        explicit Executor(RequestContext ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &frame_service,
            routing::RoutingService &routing_service,
            link::LinkService &link_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_ready_to_send_response()) {
                return ctx_.poll_send_response(frame_service, routing_service, time, rand);
            }

            if (!result_.has_value()) {
                etl::array<etl::optional<link::MediaInfo>, link::MAX_MEDIA_PORT> media_info;
                link_service.get_media_info(media_info);
                result_.emplace(media_info);
                ctx_.set_response_property(Result::Success, result_->serialized_length());
            }

            auto writer =
                POLL_UNWRAP_OR_RETURN(ctx_.poll_response_writer(frame_service, routing_service));
            POLL_UNWRAP_OR_RETURN(writer.get().serialize(*result_));
            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::media::get_media_list
