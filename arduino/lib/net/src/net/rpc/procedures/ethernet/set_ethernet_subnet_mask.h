#pragma once

#include "../../request.h"
#include "./ip_address.h"

namespace net::rpc::ethernet::set_ethernet_subnet_mask {
    struct Param {
        net::link::MediaPortNumber port_number;
        IpAddress mask;
    };

    class AsyncParameterDeserializer {
        nb::de::Bin<uint8_t> port_number_{};
        AsyncIpAddressDeserializer mask_{};

      public:
        inline Param result() const {
            return Param{
                .port_number = net::link::MediaPortNumber{port_number_.result()},
                .mask = mask_.result(),
            };
        }

        template <nb::de::AsyncReadable R>
        inline nb::Poll<nb::de::DeserializeResult> deserialize(R &readable) {
            SERDE_DESERIALIZE_OR_RETURN(port_number_.deserialize(readable));
            SERDE_DESERIALIZE_OR_RETURN(mask_.deserialize(readable));
            return nb::DeserializeResult::Ok;
        }
    };

    class Executor {
        RequestContext ctx_;
        AsyncParameterDeserializer param_{};

      public:
        explicit Executor(RequestContext ctx) : ctx_{etl::move(ctx)} {}

        nb::Poll<void> execute(
            frame::FrameService &fs,
            link::MediaService auto &ms,
            const net::local::LocalNodeService &lns,
            util::Time &time,
            util::Rand &rand
        ) {
            if (ctx_.is_response_property_set()) {
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            const auto &result = POLL_UNWRAP_OR_RETURN(ctx_.request().body().deserialize(param_));
            if (result != nb::DeserializeResult::Ok) {
                ctx_.set_response_property(Result::BadArgument, 0);
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            const auto &[port_number, mask] = param_.result();
            auto opt_ref_port = ms.get_media_port(port_number);
            if (!opt_ref_port.has_value()) {
                ctx_.set_response_property(Result::InvalidOperation, 0);
                return ctx_.poll_send_response(fs, lns, time, rand);
            }

            auto &port = opt_ref_port->get();
            link::MediaPortOperationResult op_result = port.ethernet_set_subnet_mask(mask.ip);
            switch (op_result) {
            case link::MediaPortOperationResult::Success:
                ctx_.set_response_property(Result::Success, 0);
                break;
            case link::MediaPortOperationResult::Failure:
                ctx_.set_response_property(Result::Failed, 0);
                break;
            case link::MediaPortOperationResult::UnsupportedOperation:
                ctx_.set_response_property(Result::InvalidOperation, 0);
                break;
            }

            return ctx_.poll_send_response(fs, lns, time, rand);
        }
    };
} // namespace net::rpc::ethernet::set_ethernet_subnet_mask
