#pragma once

#include "../../request.h"
#include <nb/serde.h>
#include <net/link.h>

namespace net::rpc::wifi::connect_to_access_point {
    constexpr uint8_t MAX_SSID_LENGTH = 32;
    constexpr uint8_t MAX_PASSWORD_LENGTH = 64;

    class AsyncParameterDeserializer {
        link::AsyncMediaPortNumberDeserializer media_number_;
        nb::de::Vec<nb::de::Bin<uint8_t>, MAX_SSID_LENGTH> ssid_;
        nb::de::Vec<nb::de::Bin<uint8_t>, MAX_PASSWORD_LENGTH> password_;

        struct DeserializeResult {
            link::MediaPortNumber media_number;
            etl::vector<uint8_t, MAX_SSID_LENGTH> ssid;
            etl::vector<uint8_t, MAX_PASSWORD_LENGTH> password;
        };

      public:
        DeserializeResult result() const {
            return DeserializeResult{
                .media_number = media_number_.result(),
                .ssid = ssid_.result(),
                .password = password_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        nb::Poll<nb::de::DeserializeResult> deserialize(R &r) {
            SERDE_DESERIALIZE_OR_RETURN(media_number_.deserialize(r));
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
            link::MediaService auto &ms,
            const net::local::LocalNodeService &local_node_service,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
            }

            if (!connect_success_.has_value()) {
                POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(params_));
                const auto &params = params_.result();

                auto opt_ref_port = ms.get_media_port(params.media_number);
                if (!opt_ref_port.has_value()) {
                    ctx_.set_response_property(Result::BadArgument, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                auto &port = opt_ref_port->get();
                etl::expected<nb::Poll<nb::Future<bool>>, link::MediaPortUnsupportedOperation> res =
                    port.wifi_join_ap(params.ssid, params.password, time);
                if (!res.has_value()) {
                    ctx_.set_response_property(Result::InvalidOperation, 0);
                    return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
                }

                connect_success_ = POLL_MOVE_UNWRAP_OR_RETURN(res.value());
            }

            bool success = POLL_UNWRAP_OR_RETURN(connect_success_->poll());
            auto result = success ? Result::Success : Result::Failed;
            ctx_.set_response_property(result, 0);
            return ctx_.poll_send_response(frame_service, local_node_service, time, rand);
        }
    };
} // namespace net::rpc::wifi::connect_to_access_point
