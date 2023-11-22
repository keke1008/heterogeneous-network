#pragma once

#include "../../request.h"
#include <nb/serde.h>
#include <net/link.h>

namespace net::rpc::wifi::connect_to_access_point {
    constexpr uint8_t MAX_SSID_LENGTH = 32;
    constexpr uint8_t MAX_PASSWORD_LENGTH = 64;

    class AsyncParameterDeserializer {
        nb::de::Bin<uint8_t> media_id_;
        nb::de::Vec<nb::de::Bin<uint8_t>, MAX_SSID_LENGTH> ssid_;
        nb::de::Vec<nb::de::Bin<uint8_t>, MAX_PASSWORD_LENGTH> password_;

        struct DeserializeResult {
            uint8_t media_id;
            etl::span<const uint8_t> ssid;
            etl::span<const uint8_t> password;
        };

      public:
        DeserializeResult result() const {
            return DeserializeResult{
                .media_id = media_id_.result(),
                .ssid = ssid_.result(),
                .password = password_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(media_id_.deserialize(r));
            SERDE_DESERIALIZE_OR_RETURN(ssid_.deserialize(r));
            return password_.deserialize(r);
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer params_;
        etl::optional<nb::Future<bool>> connect_success_;

      public:
        explicit Executor(RequestContext &&ctx) : ctx_{etl::move(ctx)} {
            ctx_.set_timeout_duration(WIFI_CONNECT_TO_ACCESS_POINT_TIMEOUT);
        }

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

            if (!connect_success_.has_value()) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                const auto &params = params_.result();

                auto &port = link_service.get_port(params.media_id);
                auto result = port->join_ap(params.ssid, params.password);
                if (result.has_value()) {
                    connect_success_ = POLL_MOVE_UNWRAP_OR_RETURN(result.value());
                } else {
                    ctx_.set_response_property(Result::NotSupported, 0);
                }
            }

            bool success = POLL_UNWRAP_OR_RETURN(connect_success_->poll());
            auto result = success ? Result::Success : Result::Failed;
            ctx_.set_response_property(result, 0);
            return ctx_.poll_send_response(frame_service, routing_service, time, rand);
        }
    };
} // namespace net::rpc::wifi::connect_to_access_point
